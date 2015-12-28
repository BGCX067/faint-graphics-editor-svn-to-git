// -*- coding: us-ascii-unix -*-
// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include <sstream>
#include "app/get-app-context.hh"
#include "geo/geo-func.hh"
#include "commands/blit-bitmap-cmd.hh"
#include "commands/draw-object-cmd.hh"
#include "commands/flip-rotate-cmd.hh"
#include "commands/frame-cmd.hh"
#include "commands/group-objects-cmd.hh"
#include "commands/put-pixel-cmd.hh"
#include "commands/resize-cmd.hh"
#include "commands/set-bitmap-cmd.hh"
#include "commands/set-raster-selection-cmd.hh"
#include "geo/intrect.hh"
#include "geo/rect.hh"
#include "geo/rotation.hh"
#include "objects/objellipse.hh"
#include "objects/objline.hh"
#include "objects/objraster.hh"
#include "objects/objrectangle.hh"
#include "objects/objtext.hh"
#include "objects/objtri.hh"
#include "util/auto-crop.hh"
#include "util/clipboard.hh"
#include "util/command-util.hh"
#include "util/context-commands.hh"
#include "util/frame.hh"
#include "util/file-path-util.hh"
#include "util/image-util.hh"
#include "util/image.hh"
#include "util/object-util.hh"
#include "util/pos-info.hh"
#include "util/save.hh"
#include "util/setting-util.hh"
#include "util/settings.hh"
#include "util/tool-util.hh"
#include "python/py-include.hh"
#include "python/py-interface.hh"
#include "python/py-canvas.hh"
#include "python/py-common.hh"
#include "python/py-create-object.hh"
#include "python/py-grid.hh"
#include "python/py-tri.hh"
#include "python/py-ugly-forward.hh"
#include "python/py-util.hh"
#include "python/writable-str.hh"
#include "util/cut-and-paste.hh"
#include "util/frame-props.hh"
#include "util/grid.hh"

namespace faint{

bool canvas_ok(const CanvasId& c){
  return get_app_context().Exists(c);
}

bool contains_pos(const Canvas& canvas, const IntPoint& pos){
  if (!fully_positive(pos)){
    return false;
  }
  IntSize size = canvas.GetSize();
  return pos.x < size.w && pos.y < size.h;
}

bool faint_side_ok(canvasObject* pyCanvas){
  return canvas_ok(pyCanvas->id);
}

void show_error(canvasObject*){
  PyErr_SetString(PyExc_ValueError, "Operation attempted on closed canvas.");
}

static bool py_valid_save_filename(const utf8_string& path){
  if (!is_absolute_path(path)){
    PyErr_SetString(PyExc_ValueError, "Specified path is not absolute");
    return false;
  }
  if (!is_file_path(path)){
    PyErr_SetString(PyExc_ValueError, "Specified path is not a valid file name");
    return false;
  }
  return true;
}

static PythonErrorReturn canvas_removed_error(){
  PyErr_SetString(PyExc_ValueError, "Operation attempted on closed canvas.");
  return PythonErrorReturn();
}

template<Object* (*create_func)(PyObject*)>
static PyObject* canvas_add_object(Canvas& canvas, PyObject* args){
  Object* obj = create_func(args);
  if (obj == nullptr){
    return nullptr;
  }
  python_run_command(canvas, add_object_command(obj, select_added(false)));
  return pythoned(obj, &canvas, canvas.GetImage().GetId());
}

static void canvas_center_view(Canvas& canvas, const Point& pos){
  canvas.CenterView(pos);
}

static void canvas_clear_point_overlay(Canvas& canvas){
  canvas.ClearPointOverlay();
  python_queue_refresh(canvas);
}

static void canvas_close(Canvas& canvas){
  get_app_context().Close(canvas);
}

static void canvas_context_crop(Canvas& canvas){
  python_run_command(canvas, context_crop(canvas));
}

static void canvas_context_delete(Canvas& canvas){
  context_delete(canvas, get_app_context().GetToolSettings().Get(ts_Bg));
}

static void canvas_context_flatten(Canvas& canvas){
  python_run_command(canvas, context_flatten(canvas));
}

static void canvas_context_flip_horizontal(Canvas& canvas){
  python_run_command(canvas, context_flip_horizontal(canvas));
}

static void canvas_context_offset(Canvas& canvas, const IntPoint& delta){
  python_run_command(canvas,
    context_offset(canvas, delta));
}

static void canvas_context_set_alpha(Canvas& canvas, const Common_set_alpha::color_value_t& alpha){
  python_run_command(canvas, context_set_alpha(canvas, static_cast<uchar>(alpha.GetValue())));
}

static void canvas_context_flip_vertical(Canvas& canvas){
  python_run_command(canvas, context_flip_vertical(canvas));
}

static void canvas_context_rotate90cw(Canvas& canvas){
  python_run_command(canvas, context_rotate90cw(canvas));
}

static void canvas_context_rotate(Canvas& canvas, const radian& angle){
  Paint bg(get_app_context().GetToolSettings().Get(ts_Bg));
  python_run_command(canvas, context_rotate(canvas, Angle::Rad(angle), bg));
}

static void canvas_delete_objects(Canvas& canvas, PyObject* args){
  objects_t objects;
  if (!objects_from_args(args, allow_empty(true), objects)){
    throw PresetFunctionError();
  }

  if (objects.empty()){
    return;
  }

  for (Object* obj : objects){
    if (!canvas.Has(obj->GetId())){
      throw ValueError("Atleast one object is not in this canvas.");
    }
  }
  python_run_command(canvas,
    get_delete_objects_command(objects, canvas.GetImage()));
}

static void decref_all(std::vector<smthObject*>& objs){
  for (size_t i = 0; i != objs.size(); i++){
    py_xdecref((PyObject*)objs[i]);
  }
}

static void canvas_deselect_object(Canvas& canvas, PyObject* args){
  int n = PySequence_Length(args);
  if (n == 0){
    canvas.DeselectObjects();
    python_queue_refresh(canvas);
    return;
  }

  PyObject* sequence = args;
  ScopedRef unwrappedRef;
  if (n == 1){
    PyObject* first = PySequence_GetItem(sequence, 0);
    if (PySequence_Check(first)){
      // Unwrapped the single sequence argument (allows deselect([o1,o2]))
      sequence = first;
      unwrappedRef.Reset(first);
      n = PySequence_Length(sequence);
    }
    else{
      py_xdecref(first);
    }
  }

  std::vector<smthObject*> objects;
  for (int i = 0; i != n; i++){
    ScopedRef item(PySequence_GetItem(sequence, i));
    smthObject* obj = as_smthObject(*item);
    if (obj == nullptr){
      decref_all(objects);
      throw ValueError("That's not a Faint object.");
    }
    if (!canvas.Has(obj->objectId)){
      decref_all(objects);
      throw ValueError("The canvas does not contain that item.");
    }
    item.Release();
    objects.push_back(obj);
  }

  for (size_t i = 0; i != objects.size(); ++i){
    canvas.DeselectObject(objects[i]->obj);
  }
  decref_all(objects);
  python_queue_refresh(canvas);
}

static void canvas_dwim(Canvas& canvas){
  canvas.RunDWIM();
}

static void canvas_ellipse(Canvas& canvas, const Rect& r){
  Settings s(default_ellipse_settings());
  s.Update(get_app_context().GetToolSettings());
  s.Set(ts_AntiAlias, false);
  Command* cmd = draw_object_command(its_yours(create_ellipse_object(tri_from_rect(r), s)));
  python_run_command(canvas, cmd);
}

static Optional<utf8_string> canvas_filename(Canvas& canvas){
  Optional<FilePath> filePath(canvas.GetFilePath());
  if (filePath.NotSet()){
    return no_option();
  }
  return option(filePath.Get().Str());
}

static void canvas_flatten(Canvas& canvas, PyObject* args){
  objects_t objects;
  if (!parse_objects(args, &objects)){
    throw PresetFunctionError();
  }

  const Image& image = canvas.GetImage();
  if (!has_all(image, objects)){
    throw ValueError("Objects not in this canvas");
  }
  python_run_command(canvas, get_flatten_command(objects, image));
}

static const Optional<Bitmap>& canvas_get_bitmap(Canvas& canvas){
  return canvas.GetBackground().Get<Bitmap>();
}

static PyObject* canvas_get_frame(Canvas& canvas, PyObject* args){
  if (PySequence_Length(args) == 0){
    // Return the current frame
    index_t index = canvas.GetSelectedFrame();
    return build_frame(&canvas, index);
  }

  int num;
  if (!PyArg_ParseTuple(args, "i", &num)){
    return nullptr;
  }
  const index_t frameCount = canvas.GetNumFrames();
  if (num < 0 || frameCount <= index_t(num)){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index specified.");
    return nullptr;
  }
  return build_frame(&canvas, index_t(num));
}

static PyObject* canvas_get_frames(Canvas& canvas, PyObject*){
  PyObject* list = PyList_New(0);
  const int numFrames = canvas.GetNumFrames().Get();
  for (int i = 0; i != numFrames; i++){
    PyList_Append(list, build_frame(&canvas, index_t(i)));
  }
  return list;
}

static void canvas_frame_add(Canvas& canvas, const Optional<IntSize>& maybeSize){
  IntSize size = maybeSize.Or(canvas.GetSize());
  python_run_command(canvas, add_frame_command(size));
}

static void canvas_frame_next(Canvas& canvas){
  canvas.NextFrame();
}

static void canvas_frame_previous(Canvas& canvas){
  canvas.PreviousFrame();
}

static PyObject* canvas_get_background_png_string(canvasObject* self, PyObject* args, PyObject* kwArgs){
  if (!canvas_ok(self->id)){
    return canvas_removed_error();
  }
  WritableStr stampStr("stamp");
  char* keywordList[] = {stampStr.c_str(), 0};
  int in_stamp = -1;
  PyArg_ParseTupleAndKeywords(args, kwArgs, "|p", keywordList, &in_stamp);
  if (in_stamp == -1){
    PyErr_SetString(PyExc_ValueError, "Keyword 'stamp' missing.");
    return nullptr;
  }

  const Image& image = self->canvas->GetImage();
  bool shouldStamp = in_stamp == 1;
  if (shouldStamp){
    return to_py_png_string(stamp_selection(self->canvas->GetImage()));
  }
  else{
    return image.GetBg().Visit(
      [&](const Bitmap&){
        return to_py_png_string(stamp_selection(self->canvas->GetImage()));
      },
      [](const ColorSpan&){
        // Fixme: Handle this case? (Depends on use of this function)
        PyErr_SetString(PyExc_ValueError, "Image has no bitmap.");
        return nullptr;
      });
  }
}

static std::vector<Color> canvas_get_color_list(Canvas& canvas){
  return canvas.GetImage().GetBg().Visit(
    [](const Bitmap& bmp){
      return get_palette(bmp);
    },
    [](const ColorSpan& span) -> std::vector<Color>{
      return std::vector<Color>({span.color});
    });
}

static CanvasGrid canvas_getter_grid(Canvas& self){
  return CanvasGrid(&self);
}

static int canvas_get_id(Canvas& canvas){
  return canvas.GetId().Raw();
}

static IntPoint canvas_get_max_scroll(Canvas& canvas){
  return canvas.GetMaxScrollPos();
}

static IntPoint canvas_get_mouse_pos(Canvas& canvas){
  return floored(canvas.GetRelativeMousePos());
}

static PyObject* canvas_get_objects(Canvas& canvas, PyObject*){
  const objects_t& objects = canvas.GetObjects();
  return build_object_list(objects, &canvas, canvas.GetImage().GetId());
}

static Paint canvas_get_pixel(Canvas& canvas, const IntPoint& pos){
  PosInfo posInfo(canvas.GetPosInfo(pos));
  if (outside_canvas(posInfo)){
    throw ValueError("Position outside image.");
  }

  return get_hovered_paint(posInfo,
    include_hidden_fill(false),
    include_floating_selection(true));
}

static PyObject* canvas_get_selected_objects(Canvas& canvas, PyObject*){
  const objects_t& objects = canvas.GetObjectSelection();
  PyObject* py_list = PyList_New(0);
  for (size_t i = 0; i!= objects.size(); i++){
    PyObject* smth = pythoned(objects[i], &canvas, canvas.GetImage().GetId());
    PyList_Append(py_list, smth);
  }
  return py_list;
}

static IntPoint canvas_get_scroll_pos(Canvas& canvas){
  return canvas.GetScrollPos();
}

static Optional<IntRect> canvas_get_selection(Canvas& canvas){
  const RasterSelection& selection = canvas.GetRasterSelection();
  if (selection.Empty()){
    return no_option();
  }
  return option(selection.GetRect());
}

static IntSize canvas_get_size(Canvas& canvas){
  return canvas.GetSize();
}

static coord canvas_get_zoom(Canvas& canvas){
  return canvas.GetZoom();
}

static PyObject* canvas_obj_ellipse(Canvas& canvas, PyObject* args){
  return canvas_add_object<ellipse_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_group(Canvas& canvas, PyObject* args){
  // Prevent empty groups
  if (PySequence_Length(args) == 0){
    PyErr_SetString(PyExc_TypeError, "A group must contain at least one object.");
    return nullptr;
  }

  // Use either the function arguments as the sequence of objects, or
  // a single sequence-argument as the sequence. i.e. allow both
  // Group(a, b, c, d) and Group([a,b,c,d])
  PyObject* sequence = (PySequence_Length(args) == 1 && PySequence_Check(PySequence_GetItem(args, 0))) ?
    PySequence_GetItem(args, 0) :
    args;

  const int n = PySequence_Length(sequence);
  // Prevent empty seguence arguments groups, i.e. Group([])
  if (n == 0){
    PyErr_SetString(PyExc_TypeError, "A group must contain at least one object.");
    return nullptr;
  }

  objects_t faintObjects;
  for (int i = 0; i != n; i++){
    PyObject* object = PySequence_GetItem(sequence, i);
    if (!PyObject_IsInstance(object, (PyObject*)&SmthType)){
      PyErr_SetString(PyExc_TypeError, "Unsupported item in list");
      return nullptr;
    }
    faintObjects.push_back(((smthObject*)object)->obj);
  }

  cmd_and_group_t cmd = group_objects_command(faintObjects, select_added(false));
  python_run_command(Frame(&canvas, canvas.GetImage()), cmd.first);
  return pythoned(cmd.second, &canvas, canvas.GetImage().GetId());
}

static PyObject* canvas_obj_line(Canvas& canvas, PyObject* args){
  return canvas_add_object<line_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_path(Canvas& canvas, PyObject* args){
  return canvas_add_object<path_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_polygon(Canvas& canvas, PyObject* args){
  return canvas_add_object<polygon_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_raster(Canvas& canvas, PyObject* args){
  return canvas_add_object<raster_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_rect(Canvas& canvas, PyObject* args){
  return canvas_add_object<rectangle_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_spline(Canvas& canvas, PyObject* args){
  return canvas_add_object<spline_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_text(Canvas& canvas, PyObject* args){
  return canvas_add_object<text_from_py_args>(canvas, args);
}

static PyObject* canvas_obj_tri(Canvas& canvas, PyObject* args){
  Tri tri;
  if (!parse_tri(args, &tri)){
    return nullptr;
  }

  Settings settings(default_line_settings());
  settings.Update(get_app_context().GetToolSettings());
  Object* objTri = create_tri_object(tri, settings);
  python_run_command(canvas, add_object_command(objTri, select_added(false)));
  return pythoned(objTri, &canvas, canvas.GetImage().GetId());
}

static void canvas_rect(Canvas& canvas, const Rect& r){
  Settings s(default_rectangle_settings());
  s.Update(get_app_context().GetToolSettings());
  s.Set(ts_AntiAlias, false);
  Command* cmd = draw_object_command(its_yours(create_rectangle_object(tri_from_rect(r), s)));
  python_run_command(canvas, cmd);
}

static void canvas_redo(Canvas& canvas){
  canvas.Redo();
}

static void canvas_save(Canvas& canvas, PyObject* args){
  if (PySequence_Length(args) == 0){
    // No filename specified, use the current filename
    Optional<FilePath> maybePath(canvas.GetFilePath());
    if (maybePath.NotSet()){
      throw ValueError("No filename specified, and no previously used filename available.");
    }

    FilePath filePath(maybePath.Get());
    SaveResult result = save(canvas, filePath);
    if (result.Successful()){
      canvas.NotifySaved(filePath);
      return;
    }
    else {
      throw ValueError(result.ErrorDescription());
    }
  }

  // Use the filename passed as argument
  char* filename = nullptr;
  if (!PyArg_ParseTuple(args, "s", &filename)){
    throw PresetFunctionError();
  }
  utf8_string fn_u8 = utf8_string(std::string(filename));
  if (!py_valid_save_filename(fn_u8)){
    return throw PresetFunctionError();
  }

  FilePath filePath(FilePath::FromAbsolute(fn_u8));
  SaveResult result = save(canvas, filePath);
  if (result.Successful()){
    canvas.NotifySaved(filePath);
  }
  else {
    throw ValueError(result.ErrorDescription());
  }
}

static void canvas_save_backup(Canvas& canvas, PyObject* args){
  if (PySequence_Length(args) == 0){
    throw ValueError("No backup filename specified");
  }

  char* filename = nullptr;
  if (!PyArg_ParseTuple(args, "s", &filename)){
    throw PresetFunctionError();
  }
  utf8_string fn_u8 = utf8_string(std::string(filename));
  if (!py_valid_save_filename(fn_u8)){
    throw PresetFunctionError();
  }
  FilePath filePath(FilePath::FromAbsolute(fn_u8));
  SaveResult result = save(canvas, filePath);
  if (!result.Successful()){
    throw ValueError(result.ErrorDescription());
  }
}

static void canvas_scroll_page_left(Canvas& canvas){
  canvas.ScrollPageLeft();
}

static void canvas_scroll_page_right(Canvas& canvas){
  canvas.ScrollPageRight();
}

static void canvas_scroll_page_up(Canvas& canvas){
  canvas.ScrollPageUp();
}

static void canvas_scroll_page_down(Canvas& canvas){
  canvas.ScrollPageDown();
}

static void canvas_scroll_max_up(Canvas& canvas){
  canvas.ScrollMaxUp();
}

static void canvas_scroll_max_down(Canvas& canvas){
  canvas.ScrollMaxDown();
}

static void canvas_scroll_max_left(Canvas& canvas){
  canvas.ScrollMaxLeft();
}

static void canvas_scroll_max_right(Canvas& canvas){
  canvas.ScrollMaxRight();
}

static void canvas_unsharp_mask(Canvas& canvas, const coord& sigma){
  python_run_command(canvas,
    target_full_image(get_function_command("Unsharp mask",
        [=](Bitmap& bmp){
          unsharp_mask(bmp, sigma);
        })));
}

static void canvas_select_object(Canvas& canvas, PyObject* args){
  int n = PySequence_Length(args);
  if (n == 0){
    canvas.DeselectObjects();
    get_app_context().PythonQueueRefresh(&canvas);
    return;
  }

  PyObject* sequence = args;
  if (n == 1){
    PyObject* first = PySequence_GetItem(sequence, 0);
    if (PySequence_Check(first)){
      // Unwrapped the single sequence argument (allows select([o1,o2]))
      sequence = first;
      n = PySequence_Length(sequence);
    }
  }

  std::vector<Object*> objects;
  for (int i = 0; i != n; i++){
    PyObject* item = PySequence_GetItem(sequence, i);
    smthObject* obj = as_smthObject(item);
    if (obj == nullptr){
      throw ValueError("That's not a Faint object.");
    }
    if (!canvas.Has(obj->objectId)){
      throw ValueError("The canvas does not contain that item.");
    }
    objects.push_back(obj->obj);
  }
  canvas.SelectObjects(objects, deselect_old(true));
}

static void canvas_select_frame(Canvas& self, const int& rawFrame){
  if (rawFrame < 0){
    throw ValueError("Negative frame index specified.");
  }

  index_t frameIndex(rawFrame);
  if (self.GetNumFrames() <= frameIndex){
    throw ValueError("Frame index out of range");
  }
  self.SelectFrame(frameIndex);
}

static void canvas_setter_grid(Canvas& self, const Grid& grid){
  self.SetGrid(grid);
  python_queue_refresh(self);
}

static void canvas_set_pixel(Canvas& canvas, const IntPoint& pt, const Color& c){
  if (!point_in_image(canvas.GetImage(), pt)){
    throw ValueError("Point outside image.");
  }

  python_run_command(canvas,
    get_put_pixel_command(pt, c));
}

static void canvas_set_point_overlay(Canvas& canvas, PyObject* args){
  IntPoint p;
  if (!parse_intpoint(args, &p)){
    throw PresetFunctionError();
  }
  canvas.SetPointOverlay(p);
  python_queue_refresh(canvas);
}

static void canvas_set_scroll_pos(Canvas& canvas, const IntPoint& pos){
  canvas.SetScrollPos(pos);
}

static void canvas_set_selection(Canvas& canvas, const IntRect& rect){
  if (rect.x < 0 || rect.y < 0){
    throw ValueError("The x and y coordinates of the selection rectangle must be positive.");
  }
  const RasterSelection& currentSelection(canvas.GetRasterSelection());
  python_run_command(canvas,
    get_selection_rectangle_command(rect, currentSelection));
}

static void canvas_shrink_selection(Canvas& canvas){
  const RasterSelection& selection(canvas.GetRasterSelection());

  if (selection.Empty()){
    // Select the region that auto-crop would shrink the image to.
    IntRect cropRect;
    const auto& maybeBmp = canvas.GetBackground().Get<Bitmap>();
    if (maybeBmp.NotSet()){
      return;
    }
    if (get_auto_crop_rect(maybeBmp.Get(), cropRect)){
      python_run_command(canvas,
        get_selection_rectangle_command(cropRect, selection));
    }
  }
  else if (selection.Floating()){
    // Shrink the floating bitmap and the "hole" if not copying.

    const Bitmap& floatingBmp(selection.GetBitmap());
    IntRect cropRect;
    if (get_auto_crop_rect(floatingBmp, cropRect)){
      Bitmap cropped(subbitmap(floatingBmp, cropRect));
      SelectionState newState = selection.Copying() ?
        SelectionState(cropped,
          selection.TopLeft() + cropRect.TopLeft()) :
        SelectionState(cropped,
          selection.TopLeft() + cropRect.TopLeft(),
          IntRect(selection.GetOldRect().TopLeft() + cropRect.TopLeft(),
          cropRect.GetSize()));
      python_run_command(canvas,
        set_raster_selection_command(New(newState), Old(selection.GetState()),
         "Shrink Floating Selection", false));
    }
  }
  else {
    // Shrink the selection by sort-of auto-cropping within the selected region

    IntRect selectionRect(selection.GetRect());
    const auto& maybeBmp = canvas.GetBackground().Get<Bitmap>();
    if (maybeBmp.NotSet()){
      return;
    }

    Bitmap bmpSelected(subbitmap(maybeBmp.Get(), selectionRect));
    IntRect cropRect;
    if (get_auto_crop_rect(bmpSelected, cropRect)){
      python_run_command(canvas,
        get_selection_rectangle_command(translated(cropRect, selectionRect.TopLeft()), selection));
    }
  }
}

static void canvas_set_size(Canvas& canvas, PyObject* args){
  const int n = PySequence_Length(args);
  if (n < 3){
    // No color specified
    int w = 0;
    int h = 0;
    if (!PyArg_ParseTuple(args, "ii", &w, &h)){
      throw ValueError("Two numeric arguments required");
    }
    if (w <= 0 || h <= 0){
      throw ValueError("Negative or zero width or height argument");
    }
    Paint bg = get_app_context().GetToolSettings().Get(ts_Bg);
    python_run_command(canvas, resize_command(rect_from_size(IntSize(w, h)), bg));
    return;
  }

  int w = 0;
  int h = 0;
  int r = 0;
  int g = 0;
  int b = 0;
  int a = 0;

  if (!PyArg_ParseTuple(args, "ii(iiii)", &w, &h, &r, &g, &b, &a)){
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "ii(iii)", &w, &h, &r, &g, &b)){
      throw ValueError("Two numeric arguments and and an RGB- or RGBA-color tuple is required.");
    }
    a = 255;
  }

  if (w <= 0 || h <= 0){
    throw ValueError("Negative or zero width or height argument.");
  }

  if (invalid_color(r, g, b, a)){
    throw PresetFunctionError();
  }

  python_run_command(canvas,
    resize_command(rect_from_size(IntSize(w, h)),
      Paint(color_from_ints(r,g,b,a))));
}

static void canvas_swap_frames(Canvas& canvas, const int& f0, const int& f1){
  if (f0 < 0 || f1 < 0){
    throw ValueError("Negative frame index");
  }
  int num = canvas.GetNumFrames().Get();
  if (num <= f0 || num <= f1){
    throw ValueError("Frame index out of range");
  }
  if (f0 == f1){
    return;
  }

  python_run_command(canvas,
    swap_frames_command(index_t(f0), index_t(f1)));
}

static void canvas_undo(Canvas& canvas){
  canvas.Undo();
}

static void canvas_zoom_default(Canvas& canvas){
  canvas.ZoomDefault();
}

static void canvas_zoom_fit(Canvas& canvas){
  canvas.ZoomFit();
}

static void canvas_zoom_in(Canvas& canvas){
  canvas.ZoomIn();
}

static void canvas_zoom_out(Canvas& canvas){
  canvas.ZoomOut();
}

static void canvas_pinch_whirl(Canvas& canvas, const coord& pinch, const radian& whirl){
  python_run_command(canvas,
    target_full_image(get_pinch_whirl_command(pinch, Rotation::Rad(whirl))));
}

static PyGetSetDef canvas_getseters[] = {
  {(char*)"grid",
   GET_FORWARDER(canvas_getter_grid),
   SET_FORWARDER(canvas_setter_grid),
   (char*)"A Grid bound to this canvas.", nullptr},
  {0,0,0,0,0} // Sentinel
};

static coord canvas_brightness(Canvas& canvas){
  const auto& maybeBmp(canvas.GetBackground().Get<Bitmap>());
  if (maybeBmp.NotSet()){
    throw ValueError("Image has no bitmap.");
  }
  const Bitmap& bmp(maybeBmp.Get());
  const int w = bmp.m_w;
  const int h = bmp.m_h;
  double L = 0;
  for (int x = 0; x != w; x++){
    for (int y = 0; y != h; y++){
      HSL hsl(to_hsl(get_color_raw(bmp, x,y).GetRGB()));
      L += hsl.l;
    }
  }
  L /= w * h;
  return L;
}

static void canvas_brightness_contrast(Canvas& canvas, PyObject* args){
  // Fixme: Ranges?
  brightness_contrast_t values;
  if (!PyArg_ParseTuple(args, "dd", &values.brightness, &values.contrast)){
    throw PresetFunctionError();
  }
  python_run_command(canvas,
    target_full_image(get_brightness_and_contrast_command(values)));
}

#define CANVASFWD(bundle){bundle::Name(), FORWARDER(bundle::Func<Canvas&>), bundle::ArgType(), bundle::Doc()}

// Method table for Python-Canvas interface (excluding standard
// functions like del, repr and str) Mind the per-section alphabetical
// ordering!
static PyMethodDef canvas_methods[] = {
  // Raster drawing methods
  CANVASFWD(Common_aa_line),
  CANVASFWD(Common_line),
  {"rect", FORWARDER(canvas_rect), METH_VARARGS,
   "rect(x,y,width,height)\nDraw a rectangle"},
  {"ellipse", FORWARDER(canvas_ellipse), METH_VARARGS,
   "ellipse(x,y,width,height)\nDraw an ellipse"},

  // Add-object methods
  // Fixme: Argument documentation
  {"Ellipse", FORWARDER(canvas_obj_ellipse), METH_VARARGS, "Adds an Ellipse object"},
  {"Group", FORWARDER(canvas_obj_group), METH_VARARGS, "Adds a Group"},
  {"Line", FORWARDER(canvas_obj_line), METH_VARARGS, "Adds a Line object"},
  {"Path", FORWARDER(canvas_obj_path), METH_VARARGS, "Adds a Path object"},
  {"Polygon", FORWARDER(canvas_obj_polygon), METH_VARARGS, "Adds a Polygon object"},
  {"Raster", FORWARDER(canvas_obj_raster), METH_VARARGS, "Adds a Raster object"},
  {"Rect", FORWARDER(canvas_obj_rect), METH_VARARGS, "Adds a Rectangle object"},
  {"ObjTri", FORWARDER(canvas_obj_tri), METH_VARARGS, "Adds a Tri (debug) object"},
  {"Spline", FORWARDER(canvas_obj_spline), METH_VARARGS, "Adds a Spline object"},
  {"Text", FORWARDER(canvas_obj_text), METH_VARARGS, "Text((x,y,w,h),s, settings)->Text\nAdds a Text object"},

  // Misc. methods
  {"add_frame", FORWARDER(canvas_frame_add), METH_VARARGS,
   "Adds a frame to the image"},
  CANVASFWD(Common_auto_crop),
  CANVASFWD(Common_blit),
  CANVASFWD(Common_blur),
  CANVASFWD(Common_boundary_fill),
  CANVASFWD(Common_color_balance),
  {"brightness", FORWARDER(canvas_brightness), METH_VARARGS,
   "brightness()\nThe average brightness of the image (the mean sum of the HSL-lightness)"},
  {"brightness_contrast", FORWARDER(canvas_brightness_contrast), METH_VARARGS,
   "brightness_contrast(brightness,contrast)\nApplies brightness and contrast (also known as bias and gain)."},
  {"center", FORWARDER(canvas_center_view), METH_VARARGS,
   "center(x,y)\nCenters the view at image position (x,y)"},
  CANVASFWD(Common_clear),
  CANVASFWD(Common_count_colors),
  CANVASFWD(Common_gaussian_blur),
  {"swap_frames", FORWARDER(canvas_swap_frames), METH_VARARGS,
   "swap_frames(i1,i2)\nSwaps the frames with indexes i1 and i2"},
  {"close", FORWARDER(canvas_close), METH_NOARGS,
   "close()\nClose the canvas"},
  {"context_crop", FORWARDER(canvas_context_crop), METH_NOARGS,
   "context_crop()\nCrops the image to the raster selection, crops a selected object or performs an autocrop."},
  {"context_delete", FORWARDER(canvas_context_delete), METH_NOARGS,
   "context_delete()\nDelete the selected object or raster region"},
  {"context_flip_horizontal", FORWARDER(canvas_context_flip_horizontal), METH_NOARGS,
   "context_flip_horizontal()\nFlips the image or selection horizontally"},
  {"context_set_alpha", FORWARDER(canvas_context_set_alpha), METH_VARARGS,
   "context_set_alpha(alpha)"},

  {"context_offset", FORWARDER(canvas_context_offset), METH_VARARGS,
    "context_offset(dx, dy)\nOffsets the selected objects or raster selection by dx, dy. Scrolls the image if no selection is available."},

  {"context_flip_vertical", FORWARDER(canvas_context_flip_vertical), METH_NOARGS,
   "context_flip_vertical()\nFlips the image or selection vertically"},
  {"context_rotate_90CW", FORWARDER(canvas_context_rotate90cw), METH_NOARGS,
   "context_rotate_90CW()\nRotates the image or selection 90 degrees clock-wise"},
  {"context_rotate", FORWARDER(canvas_context_rotate), METH_VARARGS,
   "context_rotate(radians)\nRotates the image or selection by radians"},
  CANVASFWD(Common_quantize),
  CANVASFWD(Common_rotate),
  {"clear_point_overlay", FORWARDER(canvas_clear_point_overlay), METH_NOARGS,
   "clear_point_overlay()\n->Clear indicated point (see set_point_overlay)."},
  {"context_flatten", FORWARDER(canvas_context_flatten), METH_NOARGS,
   "context_flatten()\nFlattens all objects or all selected objects into the background (rasterize)."},
  CANVASFWD(Common_desaturate),
  CANVASFWD(Common_sepia),
  CANVASFWD(Common_desaturate_weighted),
  CANVASFWD(Common_copy_rect),
  {"delete_objects", FORWARDER(canvas_delete_objects), METH_VARARGS,
   "delete_objects(objects)\nDeletes the objects in the sequence from the image"},
  CANVASFWD(Common_erase_but_color),
  {"flatten", FORWARDER(canvas_flatten), METH_VARARGS,
   "flatten(object1[, object2, ...])\nFlattens the specified objects."},
  {"get_bitmap", FORWARDER(canvas_get_bitmap), METH_NOARGS,
   "get_bitmap()->bmp\nReturns a copy of the pixel data in the active frame as an ifaint.Bitmap"},
  {"get_frames", FORWARDER(canvas_get_frames), METH_NOARGS,
   "get_frames() -> [frame1, frame2,...]\nReturns the frames in this image."},
  {"get_frame", FORWARDER(canvas_get_frame), METH_VARARGS,
   "get_frame([index]) -> Frame\nReturns the frame with the specified index, or the selected frame if no index specified."},
  {"get_filename", FORWARDER(canvas_filename), METH_NOARGS,
   "get_filename() -> filename\nReturns the filename, if the image has been saved."},
  {"get_id", FORWARDER(canvas_get_id), METH_NOARGS,
   "get_id() -> canvas_id\nReturns the id that identifies the canvas in this Faint session"},
  {"get_objects", FORWARDER(canvas_get_objects), METH_NOARGS, "get_objects() -> [ object1, object2, ... ]\nReturns the objects in the active frame of the image, sorted from rearmost to frontmost"},
  {"get_mouse_pos", FORWARDER(canvas_get_mouse_pos), METH_NOARGS,
   "get_mouse_pos() -> (x,y)\nReturns the mouse position relative to the image"},
  {"get_scroll_pos", FORWARDER(canvas_get_scroll_pos), METH_VARARGS,
   "get_scroll_pos() -> x, y\nReturns the scroll bar positions"},
  {"get_selection", FORWARDER(canvas_get_selection), METH_NOARGS,
   "get_selection() -> (x,y,w,h)\nReturns the selection rectangle or None"},
  {"get_selected", FORWARDER(canvas_get_selected_objects), METH_NOARGS, "get_sel_obj() -> [ object1, object2, ... ]\nReturns a list of the currently selected objects"},
  {"get_zoom", FORWARDER(canvas_get_zoom), METH_NOARGS,
   "get_zoom() -> zoom\nReturns the current zoom as a floating point value"},
  CANVASFWD(Common_insert_bitmap),
  CANVASFWD(Common_paste),
  {"set_point_overlay", FORWARDER(canvas_set_point_overlay), METH_VARARGS,
   "set_point_overlay(x,y)\n->Indicate a point."},
  CANVASFWD(Common_invert),
  {"get_colors", FORWARDER(canvas_get_color_list), METH_NOARGS,
   "get_colors()->[c1, c2, ..., cn]\nReturns a list of the unique colors used in the image."},
  {"get_size", FORWARDER(canvas_get_size), METH_NOARGS,
   "get_size() -> (w,h)\nReturns the size of the image."},
  {"next_frame", FORWARDER(canvas_frame_next), METH_NOARGS,
   "next_frame()\nSelects the next frame"},
  {"prev_frame", FORWARDER(canvas_frame_previous), METH_NOARGS,
   "prev_frame()\nSelects the previous frame"},
  {"redo", FORWARDER(canvas_redo), METH_NOARGS,
   "redo()\nRedo the last undone action"},
  CANVASFWD(Common_replace_color),
  CANVASFWD(Common_replace_alpha),
   {"save", FORWARDER(canvas_save), METH_VARARGS,
   "save(filename)\nSave the image to file"},
  {"save_backup", FORWARDER(canvas_save_backup), METH_VARARGS,
   "save_backup(filename)\nSave a copy of the image to the specified file without changing the target filename for the image"},
  CANVASFWD(Common_scale_bilinear),
  CANVASFWD(Common_scale_nearest),
  {"select", FORWARDER(canvas_select_object), METH_VARARGS,
   "select(object(s))\nSelects the object or list of objects specified.\nThe previous selection will be discarded"},
  {"deselect", FORWARDER(canvas_deselect_object), METH_VARARGS,
   "deselect(object(s))\nDeselects the object or list of objects."},
  CANVASFWD(Common_flood_fill),
  CANVASFWD(Common_set_alpha),
  {"set_pixel", FORWARDER(canvas_set_pixel), METH_VARARGS,
   "set_pixel(x,y,(r,g,b[,a]))\nSets the pixel x,y to the specified color"},
  {"get_max_scroll", FORWARDER(canvas_get_max_scroll), METH_VARARGS,
   "get_max_scroll() -> x,y\nReturns the largest useful scroll positions"},
  {"get_pixel", FORWARDER(canvas_get_pixel), METH_VARARGS,
   "get_pixel(x,y) -> (r,g,b,a)\nReturns the color at x,y"},
  {"get_background_png_string", FAINT_KW_TO_PY(canvas_get_background_png_string), METH_KEYWORDS | METH_VARARGS,
   "get_background_png_string(stamp=True|False) -> s\nReturns the background encoded in png as a string.\nIf the keyword argument stamp is True, any floating raster selection will be stamped onto the image."},
  {"pinch_whirl", FORWARDER(canvas_pinch_whirl), METH_VARARGS, "pinch_whirl(pinch, whirl)\nPinch (-1.0 to 1.0) and whirl (- 2pi to 2pi) the image"},
  CANVASFWD(Common_pixelize),
  CANVASFWD(Common_set_rect),
  {"select_frame", FORWARDER(canvas_select_frame), METH_VARARGS,
   "select_frame(index)\nSelects the frame with the specified index as active."},
  {"set_scroll_pos", FORWARDER(canvas_set_scroll_pos), METH_VARARGS,
   "set_scroll_pos(x, y)\nSets the scroll bar positions"},
  {"set_selection", FORWARDER(canvas_set_selection), METH_VARARGS,
   "set_selection(x,y,w,h)\nSets the selection rectangle"},
  {"shrink_selection", FORWARDER(canvas_shrink_selection), METH_VARARGS,
   "shrink_selection()\nAuto-shrink the selection rectangle to an image detail by trimming same-colored areas from its sides"},
  {"set_size", FORWARDER(canvas_set_size), METH_VARARGS,
   "set_size(w,h)\nSets the image size to w,h"},
  CANVASFWD(Common_threshold),
  {"undo", FORWARDER(canvas_undo), METH_NOARGS,
   "undo()\nUndo the last action"},
  {"scroll_page_left", FORWARDER(canvas_scroll_page_left), METH_NOARGS,
   "scroll_page_left()\nScroll the image one page to the left"},
  {"scroll_page_right", FORWARDER(canvas_scroll_page_right), METH_NOARGS,
   "scroll_page_right()\nScroll the image one page to the right"},
  {"scroll_page_up", FORWARDER(canvas_scroll_page_up), METH_NOARGS,
   "scroll_page_up()\nScroll the image one page up"},
  {"scroll_page_down", FORWARDER(canvas_scroll_page_down), METH_NOARGS,
   "scroll_page_down()\nScroll the image one page down"},
  {"scroll_max_left", FORWARDER(canvas_scroll_max_left), METH_NOARGS,
   "scroll_max_left()\nScroll the image to the left"},
  {"scroll_max_right", FORWARDER(canvas_scroll_max_right), METH_NOARGS,
   "scroll_max_right()\nScroll the image to the right"},
  {"scroll_max_up", FORWARDER(canvas_scroll_max_up), METH_NOARGS,
   "scroll_max_up()\nScroll the image up"},
  {"scroll_max_down", FORWARDER(canvas_scroll_max_down), METH_NOARGS,
   "scroll_max_down()\nScroll the image to the bottom"},
  {"unsharp_mask", FORWARDER(canvas_unsharp_mask), METH_VARARGS,
   "unsharp_mask(sigma)\nCreate an unsharp mask by subtracting a gaussian blur"},
  {"dwim", FORWARDER(canvas_dwim), METH_NOARGS,
   "dwim()\nAlternate the behavior of the last run command"},
  {"zoom_default", FORWARDER(canvas_zoom_default), METH_NOARGS,
   "zoom_default()\nSet zoom to 1:1"},
  {"zoom_fit", FORWARDER(canvas_zoom_fit), METH_NOARGS,
   "zoom_fit()\nZoom image to fit view"},
  {"zoom_in", FORWARDER(canvas_zoom_in), METH_NOARGS,
   "zoom_in()\nZoom in one step"},
  {"zoom_out", FORWARDER(canvas_zoom_out), METH_NOARGS,
   "zoom_out()\nZoom out one step"},
  {0,0,0,0}  // Sentinel
};

// Python standard methods follow...
static PyObject* canvas_new(PyTypeObject* type, PyObject*, PyObject*){
  canvasObject *self;
  self = (canvasObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* canvas_repr(canvasObject* self){
  std::stringstream ss;
  if (canvas_ok(self->id)){
    ss << "Canvas #" << self->id.Raw();
    Optional<FilePath> filePath(self->canvas->GetFilePath());
    if (filePath.IsSet()){
      ss << " " << filePath.Get().Str();
    }
  }
  else{
    ss << "Retired Canvas #" << self->id.Raw();
  }
  return build_unicode(utf8_string(ss.str()));
}

static Py_hash_t canvas_hash(PyObject* selfRaw){
  canvasObject* self((canvasObject*)selfRaw);
  return self->canvas->GetId().Raw();
}

static PyObject* canvas_richcompare(canvasObject* self, PyObject* otherRaw, int op){
  if (!PyObject_IsInstance(otherRaw, (PyObject*)&CanvasType)){
    Py_RETURN_NOTIMPLEMENTED;
  }
  canvasObject* other((canvasObject*)otherRaw);
  return py_rich_compare(self->canvas->GetId(), other->canvas->GetId(), op);
}

// General canvas init error
static const char* const g_canvasInitError = "Incorrect parameters. Canvas supports: Canvas(width, height)\nCanvas(width, height, (r,g,b[,a]))\nCanvas(filename(s))";

static int canvas_init_from_filename_sequence(canvasObject* self, const std::vector<utf8_string>& path_vec){
  FileList filePaths;
  for (const utf8_string& path : path_vec){
    if (!is_absolute_path(path)){
      py_set_value_error(space_sep("Error:", quoted(path), "is relative."));
      return init_fail;
    }
    if (!is_file_path(path)){
      py_set_value_error(space_sep("Error:", quoted(path),
          "is not a valid file path."));
      return init_fail;
    }
    filePaths.push_back(FilePath::FromAbsolute(path));
  }

  assert(!filePaths.empty());

  if (filePaths.size() == 1){
    Canvas* canvas = get_app_context().Load(filePaths.back(), change_tab(true));
    if (canvas == nullptr){
      py_set_value_error(utf8_string("Failed loading: ") +
        filePaths.back().Str());
      return init_fail;
    }
    self->canvas = canvas;
    self->id = self->canvas->GetId();
    return init_ok;
  }

  assert(filePaths.size() > 1);
  Canvas* canvas = get_app_context().LoadAsFrames(filePaths, change_tab(true));
  if (canvas == nullptr){
    py_set_value_error("Failed loading."); // Fixme: Indicate failed filename (not possible here, not available)
    return init_fail;
  }
  self->canvas = canvas;
  self->id = self->canvas->GetId();
  return init_ok;
}

static int canvas_init_no_args(canvasObject* self){
  AppContext& ctx = get_app_context();
  self->canvas = &(ctx.NewDocument(ctx.GetDefaultImageInfo()));
  self->id = self->canvas->GetId();
  return init_ok;
}

// Canvas-constructor, adds a new canvas to faint
static int canvas_init(canvasObject* self, PyObject* args, PyObject*){
  const int n = PySequence_Length(args);

  if (n == 0){
    return canvas_init_no_args(self);
  }
  else if (n == 1){
    ScopedRef ref(PySequence_GetItem(args, 0));
    if (PyUnicode_Check(*ref)){
      Optional<utf8_string> maybeStr = parse_py_unicode(*ref);
      if (maybeStr.NotSet()){
        return init_fail;
      }
      return canvas_init_from_filename_sequence(self, {maybeStr.Get()});
    }
    else if (PySequence_Check(*ref)){
      return canvas_init_from_filename_sequence(self, parse_string_sequence(*ref));
    }
  }
  else if (is_string_sequence(args)){
    return canvas_init_from_filename_sequence(self, parse_string_sequence(args));
  }
  else if (n == 2){
    int w, h;
    if (!PyArg_ParseTuple(args, "ii", &w, &h)){
      PyErr_SetString(PyExc_TypeError, g_canvasInitError);
      return init_fail;
    }
    if (w <= 0 || h <= 0){
      PyErr_SetString(PyExc_ValueError, "Zero or negative Width or height passed to Canvas constructor");
      return init_fail;
    }
    self->canvas = &(get_app_context().NewDocument(ImageInfo(IntSize(w, h), create_bitmap(false))));
    self->id = self->canvas->GetId();
    return init_ok;
  }
  else if (n == 3){
    int w, h, r, g, b;
    int a = 255;
    bool rgb = PyArg_ParseTuple(args, "ii(iii)", &w, &h, &r, &g, &b) != 0;
    if (!rgb){
      PyErr_Clear();
      bool rgba = PyArg_ParseTuple(args, "ii(iiii)", &w, &h, &r, &g, &b, &a) != 0;
      if (!rgba){
        PyErr_SetString(PyExc_TypeError, g_canvasInitError);
        return init_fail;
      }
    }
    if (w <= 0 || h <= 0){
      PyErr_SetString(PyExc_ValueError, "Negative width or height passed to Canvas constructor");
      return init_fail;
    }
    if (invalid_color(r,g,b,a)){
      return init_fail;
    }
    self->canvas = &(get_app_context().NewDocument(ImageInfo(IntSize(w,h), color_from_ints(r,g,b,a), create_bitmap(false))));
    self->id = self->canvas->GetId();
    return init_ok;
  }
  PyErr_SetString(PyExc_TypeError, g_canvasInitError);
  return init_fail;
}

PyTypeObject CanvasType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "Canvas", // tp_name
  sizeof(canvasObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // reserved (formerly tp_compare)
  (reprfunc)canvas_repr, // tp_repr
  0, // tp_as_number
  0, // tp_as_sequence
  0, // tp_as_mapping
  canvas_hash, // tp_hash
  0, // tp_call
  0, // tp_str
  0, // tp_getattro
  0, // tp_setattro
  0, // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
  "An image in a Faint tab.\n\nConstruction options:\n Canvas(w,h)\n Canvas(w,h,(r,g,b[,a]))\n Canvas(filename)\n\n - All opened images can be retrieved with the list_images() function or the images-list.\n", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  (richcmpfunc)canvas_richcompare, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  canvas_methods, // tp_methods
  0, // tp_members
  canvas_getseters, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)canvas_init, // tp_init
  0, // tp_alloc
  canvas_new, // tp_new
  0, // tp_free
  0, // tp_is_gc
  0, // tp_bases
  0, // tp_mro
  0, // tp_cache
  0, // tp_subclasses
  0, // tp_weaklist
  0, // tp_del
  0, // tp_version_tag
  0  // tp_finalize
};

PyObject* pythoned(Canvas& canvas){
  canvasObject* py_canvas = (canvasObject*)CanvasType.tp_alloc(&CanvasType, 0);
  py_canvas->canvas = &canvas;
  py_canvas->id = py_canvas->canvas->GetId();
  return (PyObject*) py_canvas;
}

}
