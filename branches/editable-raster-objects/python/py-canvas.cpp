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
#include "app/getappcontext.hh"
#include "commands/frame-cmd.hh"
#include "commands/blit-bitmap-cmd.hh"
#include "commands/draw-object-cmd.hh"
#include "commands/flip-rotate-cmd.hh"
#include "commands/group-objects-cmd.hh"
#include "commands/resize-cmd.hh"
#include "commands/set-bitmap-cmd.hh"
#include "commands/set-pixel-cmd.hh"
#include "geo/grid.hh"
#include "objects/objellipse.hh"
#include "objects/objline.hh"
#include "objects/objraster.hh"
#include "objects/objrectangle.hh"
#include "objects/objtext.hh"
#include "objects/objtri.hh"
#include "python/pythoninclude.hh"
#include "python/py-canvas.hh"
#include "python/py-create-object.hh"
#include "python/py-grid.hh"
#include "python/py-util.hh"
#include "python/pyinterface.hh"
#include "python/writable-str.hh"
#include "util/autocrop.hh"
#include "util/clipboard.hh"
#include "util/commandutil.hh"
#include "util/context-commands.hh"
#include "util/cursorposinfo.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/imageutil.hh"
#include "util/objutil.hh"
#include "util/pathutil.hh"
#include "util/save.hh"
#include "util/settings.hh"
#include "util/settingutil.hh"
#include "util/util.hh"
#include "util/toolutil.hh"

class PythonErrorReturn{
public:
  operator int() const{
    return faint::init_fail;
  }
  operator PyObject*() const{
    return nullptr;
  }
};

static bool py_valid_save_filename( const faint::utf8_string& path ){
  if ( !faint::is_absolute_path(path) ){
    PyErr_SetString(PyExc_ValueError, "Specified path is not absolute");
    return false;
  }
  if ( !faint::is_file_path(path) ){
    PyErr_SetString(PyExc_ValueError, "Specified path is not a valid file name");
    return false;
  }
  return true;
}

PythonErrorReturn canvas_removed_error();

template<Object* (*Creator)(PyObject*)>
static PyObject* canvas_add_object(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Object* obj = Creator( args );
  if ( obj == nullptr ){
    return nullptr;
  }
  faint::python_run_command(self->canvas, get_add_object_command(obj, select_added(false)));
  return pythoned(obj, self->canvas, self->canvas->GetImage().GetId());
}

static PyObject* canvas_auto_crop(canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Command* cmd = get_auto_crop_command(self->canvas->GetImage());
  if ( cmd != nullptr ){
    faint::python_run_command( self->canvas, cmd );
  }
  return Py_BuildValue("");
}

PyObject* canvas_blit( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  int x, y;
  PyObject* raw_bmp;
  if ( !PyArg_ParseTuple(args, "iiO", &x, &y, &raw_bmp) ){
    return nullptr;
  }

  faint::Bitmap* bmp = as_Bitmap(raw_bmp);
  if ( bmp == nullptr ){
    return nullptr;
  }

  faint::python_run_command(self->canvas,
    get_blit_bitmap_command(IntPoint(x,y), *bmp));
  return Py_BuildValue("");
}

PyObject* canvas_blur( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command(self->canvas, get_blur_command());
  return Py_BuildValue("");
}

PyObject* canvas_center_view( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Point out;
  if ( !parse_point(args, &out) ){
    return nullptr;
  }
  self->canvas->CenterView( out );
  return Py_BuildValue("");
}

PyObject* canvas_copy_rect( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  IntRect rect;
  if ( !parse_intrect(args, &rect) ){
    return nullptr;
  }
  const faint::Bitmap& srcBmp(self->canvas->GetBitmap());
  if ( !faint::inside(rect, srcBmp) ){
    PyErr_SetString(PyExc_ValueError, "Rectangle not fully inside image.");
    return nullptr;
  }
  faint::Bitmap bmp(sub_bitmap(srcBmp, rect));
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    PyErr_SetString(PyExc_OSError, "Failed opening clipboard.");
    return nullptr;
  }

  // If the bg-color is a color, use it as the background for
  // blending alpha when pasting outside Faint.
  faint::DrawSource src(GetAppContext().GetToolSettings().Get(ts_BgCol));
  if ( src.IsColor() ){
    clipboard.SetBitmap(bmp, strip_alpha(src.GetColor()));
  }
  else{
    clipboard.SetBitmap(bmp);
  }
  return Py_BuildValue("");
}

PyObject* canvas_clear_point_overlay(canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ClearPointOverlay();
  faint::python_queue_refresh(self->canvas);
  return Py_BuildValue("");
}

static PyObject* canvas_close(canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  GetAppContext().Close( *( self->canvas ) );
  return Py_BuildValue("");
}

static PyObject* canvas_context_crop( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command( self->canvas,
    context_crop(*self->canvas) );
  return Py_BuildValue( "" );
}

static PyObject* canvas_context_delete( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ContextDelete();
  return Py_BuildValue("");
}

PyObject* canvas_context_flatten( canvasObject* self, PyObject* /*args*/ ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command( self->canvas,
    context_flatten(*self->canvas) );
  return Py_BuildValue("");
}

static PyObject* canvas_context_flip_horizontal(canvasObject* self, PyObject* ){
  // Fixme: Generate forwarding functions like this one.
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }

  faint::python_run_command( self->canvas,
    context_flip_horizontal(*self->canvas) );
  return Py_BuildValue("");
}

static PyObject* canvas_context_offset(canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }

  IntPoint delta;
  if ( !parse_intpoint(args, &delta) ){
    return nullptr;
  }
  faint::python_run_command(self->canvas,
    context_offset(*self->canvas, delta));

  return Py_BuildValue("");
}

static PyObject* canvas_context_flip_vertical(canvasObject* self, PyObject* ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command(self->canvas,
    context_flip_vertical(*self->canvas));
  return Py_BuildValue("");
}

static PyObject* canvas_context_rotate90cw(canvasObject* self, PyObject* ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command(self->canvas,
    context_rotate90cw(*self->canvas));
  return Py_BuildValue("");
}

static PyObject* canvas_rotate(canvasObject* self, PyObject* args){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  faint::radian angle;
  if ( !parse_radian(args, &angle) ){
    return nullptr;
  }

  faint::python_run_command(self->canvas,
    rotate_image_command(angle, faint::color_white()));
  return Py_BuildValue("");
}

static PyObject* canvas_delete_objects( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  objects_t objects;
  if ( !objects_from_args( args, objects ) ){
    return nullptr;
  }
  CanvasInterface* canvas = self->canvas;
  for ( size_t i = 0; i != objects.size(); i++ ){
    if ( !canvas->Has( objects[i]->GetId() ) ){
      PyErr_SetString(PyExc_ValueError, "Atleast one object is not in this canvas.");
      return nullptr;
    }
  }

  faint::python_run_command( canvas,
    get_delete_objects_command( objects, canvas->GetImage() ) );
  return Py_BuildValue("");
}

static void decref_all( std::vector<smthObject*>& objs ){
  for ( size_t i = 0; i != objs.size(); i++ ){
    faint::py_xdecref((PyObject*)objs[i]);
  }
}

static PyObject* canvas_deselect_object( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  int n = PySequence_Length( args );
  if ( n == 0 ){
    self->canvas->DeselectObjects();
    return Py_BuildValue("");
  }

  PyObject* sequence = args;
  faint::ScopedRef unwrappedRef;
  if ( n == 1 ){
    PyObject* first = PySequence_GetItem( sequence, 0 );
    if ( PySequence_Check( first ) ){
      // Unwrapped the single sequence argument (allows deselect([o1,o2]))
      sequence = first;
      unwrappedRef.Reset(first);
      n = PySequence_Length( sequence );
    }
    else{
      faint::py_xdecref(first);
    }
  }

  std::vector<smthObject*> objects;
  for ( int i = 0; i != n; i++ ){
    faint::ScopedRef item(PySequence_GetItem( sequence, i ));
    smthObject* obj = as_smthObject( *item );
    if ( obj == nullptr ){
      decref_all(objects);
      PyErr_SetString(PyExc_TypeError, "That's not a Faint object.");
      return nullptr;
    }
    if ( !self->canvas->Has( obj->objectId ) ){
      decref_all(objects);
      PyErr_SetString(PyExc_ValueError, "The canvas does not contain that item.");
      return nullptr;
    }
    item.Release();
    objects.push_back(obj);
  }

  for ( size_t i = 0; i != objects.size(); ++i ){
    self->canvas->DeselectObject(objects[i]->obj);
  }
  decref_all(objects);
  faint::python_queue_refresh(self->canvas);
  return Py_BuildValue("");
}

static PyObject* canvas_desaturate( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command(self->canvas, get_desaturate_simple_command() );
  return Py_BuildValue("");
}

static PyObject* canvas_desaturate_weighted( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command(self->canvas, get_desaturate_weighted_command());
  return Py_BuildValue("");
}

static PyObject* canvas_dwim(canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->RunDWIM();
  return Py_BuildValue("");
}

static PyObject* canvas_ellipse(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Rect r;
  if ( !parse_rect( args, &r ) ){
    PyErr_SetString( PyExc_TypeError, "Invalid parameters to ellipse(x, y, w, h)" );
    return nullptr;
  }
  Settings s( default_ellipse_settings() );
  s.Update( GetAppContext().GetToolSettings() );
  s.Set( ts_AntiAlias, false );
  Command* cmd = new DrawObjectCommand(its_yours(new ObjEllipse(tri_from_rect(r), s)));
  faint::python_run_command(self->canvas, cmd);
  return Py_BuildValue("");
}

PyObject* canvas_erase_but_color( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  faint::Color keep;
  faint::DrawSource eraser;
  if ( PySequence_Length(args) == 1 ){
    PyObject* pyKeep = PySequence_GetItem(args, 0);
    bool ok = parse_color( pyKeep, keep );
    faint::py_xdecref(pyKeep);
    if ( !ok ){
      return nullptr;
    }
    eraser = GetAppContext().GetToolSettings().Get( ts_BgCol );
  }
  else if ( PySequence_Length(args) == 2 ){
    PyObject* pyKeep = PySequence_GetItem(args, 0);
    PyObject* pyEraser = PySequence_GetItem(args, 1);
    bool colorsOK = parse_color(pyEraser, keep ) &&
      parse_draw_source( pyEraser, eraser );
    faint::py_xdecref(pyKeep);
    faint::py_xdecref(pyEraser);
    if ( !colorsOK ){
      return nullptr;
    }
  }
  if ( keep == eraser ){
    PyErr_SetString(PyExc_ValueError, "Same erase color as the kept color");
    return nullptr;
  }
  faint::python_run_command(self->canvas, get_erase_but_color_command(keep, eraser));
  return Py_BuildValue("");
}

static PyObject* canvas_filename( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Optional<faint::FilePath> filePath( self->canvas->GetFilePath() );
  if ( filePath.NotSet() ){
    return Py_BuildValue("");
  }
  faint::utf8_string filePathUtf8(to_utf8(filePath.Get()));
  return Py_BuildValue( "s", filePathUtf8.c_str() );
}

static PyObject* canvas_flatten( canvasObject* self, PyObject* args ){
  if (!canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  objects_t objects;
  if ( !parse_objects(args, &objects) ){
    return nullptr;
  }

  const faint::Image& image = self->canvas->GetImage();
  if ( !faint::has_all(image, objects) ){
    PyErr_SetString(PyExc_ValueError, "Objects not in this canvas");
    return nullptr;
  }
  faint::python_run_command(self->canvas, get_flatten_command(objects, image));
  return Py_BuildValue("");
}

static PyObject* canvas_get_bitmap( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  return build_bitmap(self->canvas->GetBitmap());
}

static PyObject* canvas_get_frame( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }

  if ( PySequence_Length(args) == 0 ){
    // Return the current frame
    index_t index = self->canvas->GetSelectedFrame();
    return build_frame(self->canvas, index);
  }

  int num;
  if ( !PyArg_ParseTuple(args, "i", &num) ){
    return nullptr;
  }
  const index_t frameCount = self->canvas->GetNumFrames();
  const index_t index((size_t)num);
  if ( num < 0 || frameCount <= index ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index specified.");
    return nullptr;
  }
  return build_frame(self->canvas, index);
}

static PyObject* canvas_get_frames( canvasObject* self ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }

  PyObject* list = PyList_New(0);
  const size_t numFrames = self->canvas->GetNumFrames().Get();
  for ( size_t i = 0; i != numFrames; i++ ){
    PyList_Append(list, build_frame(self->canvas, index_t(i)));
  }
  return list;
}

static PyObject* canvas_frame_add( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }

  IntSize size;
  if ( PySequence_Length(args) == 0 ){
    size = self->canvas->GetSize();
  }
  else if ( !PyArg_ParseTuple(args, "ii", &size.w, &size.h ) ){
    return nullptr;
  }
  faint::python_run_command(self->canvas, add_frame_command(size));
  return Py_BuildValue("");
}

static PyObject* canvas_frame_get_delay( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }
  int num;
  if ( !PyArg_ParseTuple(args, "i", &num ) ){
    return nullptr;
  }
  if ( num < 0 || self->canvas->GetNumFrames().Get() <= static_cast<size_t>(num) ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index");
    return nullptr;
  }
  delay_t delay(self->canvas->GetFrame(index_t(num)).GetDelay());
  return Py_BuildValue("i", delay.Get());
}

static PyObject* canvas_frame_get_hotspot( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }
  int num;
  if ( !PyArg_ParseTuple(args, "i", &num ) ){
    return nullptr;
  }
  if ( num < 0 || self->canvas->GetNumFrames().Get() <= static_cast<size_t>(num) ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index");
    return nullptr;
  }

  IntPoint pos(self->canvas->GetFrame(index_t(num)).GetHotSpot());
  return build_intpoint(pos);
}

static PyObject* canvas_frame_next( canvasObject* self, PyObject* ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->NextFrame();
  return Py_BuildValue("");
}

static PyObject* canvas_frame_previous( canvasObject* self, PyObject* ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->PreviousFrame();
  return Py_BuildValue("");
}

static PyObject* canvas_frame_set_delay( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }
  int num, delay;
  if ( !PyArg_ParseTuple(args, "ii", &num, &delay ) ){
    return nullptr;
  }
  if ( delay < 0 ){ // Fixme: -1 should probably mean 'forever' for gifs.
    PyErr_SetString(PyExc_ValueError, "Negative delay");
    return nullptr;
  }
  if ( num < 0 || self->canvas->GetNumFrames().Get() <= static_cast<size_t>(num) ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index");
    return nullptr;
  }

  delay_t oldDelay(self->canvas->GetFrame(index_t(num)).GetDelay());
  delay_t newDelay(delay);

  faint::python_run_command(self->canvas,
    set_frame_delay_command(index_t(num), New(newDelay), Old(oldDelay)));
  return Py_BuildValue("");
}

static PyObject* canvas_frame_set_hotspot( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }
  int num, x, y;
  if ( !PyArg_ParseTuple(args, "i(ii)", &num, &x, &y ) ){
    return nullptr;
  }

  if ( num < 0 || self->canvas->GetNumFrames().Get() <= static_cast<size_t>(num) ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index");
    return nullptr;
  }


  index_t index(num);
  IntPoint oldHotSpot( self->canvas->GetFrame(index).GetHotSpot() );
  IntPoint newHotSpot(x,y);
  faint::python_run_command(self->canvas,
    set_frame_hotspot_command(index, New(newHotSpot), Old(oldHotSpot)));
  return Py_BuildValue("");
}

static PyObject* canvas_get_background_png_string( canvasObject* self, PyObject* args, PyObject* kwArgs ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  faint::WritableStr stampStr("stamp");
  char* keywordList[] = {stampStr.c_str(), 0};
  int in_stamp = -1;
  PyArg_ParseTupleAndKeywords(args, kwArgs, "|i", keywordList, &in_stamp); // Fixme: Replace with p for Python 3
  if ( in_stamp == -1 ){
    PyErr_SetString(PyExc_ValueError, "Keyword 'stamp' missing.");
    return nullptr;
  }
  bool shouldStamp = in_stamp == 1;
  return shouldStamp ?
    to_py_png_string(stamp_selection(self->canvas->GetImage())) :
    to_py_png_string(self->canvas->GetImage().GetBitmap());
}

static PyObject* canvas_get_color_list( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  const faint::Bitmap& bmp = self->canvas->GetImage().GetBitmap();
  std::vector<faint::Color> colors( get_palette(bmp) );
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i != colors.size(); i++ ){
    PyList_Append( py_list, build_color_tuple(colors[i]) );
  }
  return py_list;
}

static PyObject* canvas_get_grid( canvasObject* self, void* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  return py_grid_canvas( self->canvas );
}

static PyObject* canvas_get_id( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  return Py_BuildValue( "i", self->id.Raw() );
}

static PyObject* canvas_get_max_scroll( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  return build_intpoint(self->canvas->GetMaxScrollPos());
}

static PyObject* canvas_get_mouse_pos( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  IntPoint p( floored(self->canvas->GetRelativeMousePos()) );
  return Py_BuildValue("ii", p.x, p.y);
}

static PyObject* canvas_get_objects( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  const objects_t& objects = self->canvas->GetObjects();
  return build_object_list(objects, self->canvas, self->canvas->GetImage().GetId());
}

static PyObject* canvas_get_pixel( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  IntPoint pos;
  if ( !parse_intpoint(args, &pos) ){
    return nullptr;
  }
  const faint::Bitmap& bmp = self->canvas->GetImage().GetBitmap();
  if ( invalid_pixel_pos(pos, bmp) ){
    return nullptr;
  }
  // To include objects, the CursorPositionInfo must be retrieved.
  // (As an alternative, one could draw the background and all objects
  // for the pixel, and then fetch it but this would give blended
  // colors for edges of objects, as well as the blended color for
  // alpha-objects, which may be undesired)
  CursorPositionInfo posInfo(self->canvas->GetPosInfo(pos));
  faint::DrawSource src = get_hovered_draw_source(posInfo, hidden_fill(false));
  return build_draw_source(src);
}

PyObject* canvas_get_selected_objects( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  const objects_t& objects = self->canvas->GetObjectSelection();
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i!= objects.size(); i++ ){
    PyObject* smth = pythoned(objects[i], self->canvas, self->canvas->GetImage().GetId());
    PyList_Append( py_list, smth );
  }
  return py_list;
}

PyObject* canvas_get_scroll_pos( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  return build_intpoint(self->canvas->GetScrollPos());
}

PyObject* canvas_get_selection( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  IntRect r = self->canvas->GetRasterSelection().GetRect();
  if ( r.w == 0 || r.h == 0 ){
    return Py_BuildValue("");
  }
  return Py_BuildValue("iiii", r.x, r.y, r.w, r.h );
}

static PyObject* canvas_get_size( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  IntSize size( self->canvas->GetSize() );
  return build_intsize(size);
}

static PyObject* canvas_get_zoom(canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  return build_coord(self->canvas->GetZoom());
}

PyObject* canvas_invert( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command( self->canvas, get_invert_command() );
  return Py_BuildValue("");
}

static PyObject* canvas_aa_line(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Point p0, p1;
  if ( !parse_point2(args, &p0, &p1 ) ){
    PyErr_SetString( PyExc_TypeError, "Invalid parameters to aa_line(x0, y0, x1, y1)");
    return nullptr;
  }
  Settings s( default_line_settings() );
  s.Update( GetAppContext().GetToolSettings() );
  Command* cmd = get_aa_line_command(floored(p0), floored(p1), s.Get(ts_FgCol).GetColor());
  faint::python_run_command(self->canvas, cmd);
  return Py_BuildValue("");
}

static PyObject* canvas_line(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Point p0, p1;
  if ( !parse_point2( args, &p0, &p1) ){
    PyErr_SetString( PyExc_TypeError, "Invalid parameters to line( x0, y0, x1, y1 )" );
    return nullptr;
  }

  Settings s( default_line_settings() );
  s.Update( GetAppContext().GetToolSettings() );
  s.Set( ts_AntiAlias, false );

  Command* cmd = new DrawObjectCommand(its_yours(new ObjLine(Points(vector_of(p0, p1)), s)));
  faint::python_run_command(self->canvas, cmd);
  return Py_BuildValue("");
}

static PyObject* canvas_obj_ellipse( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateEllipse>(self, args);
}

static PyObject* canvas_obj_group( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  // Prevent empty groups
  if ( PySequence_Length( args ) == 0 ){
    PyErr_SetString( PyExc_TypeError, "A group must contain at least one object." );
    return nullptr;
  }

  // Use either the function arguments as the sequence of objects, or
  // a single sequence-argument as the sequence. i.e. allow both
  // Group( a, b, c, d ) and Group( [a,b,c,d] )
  PyObject* sequence = ( PySequence_Length( args ) == 1 && PySequence_Check( PySequence_GetItem( args, 0 ) ) ) ?
    PySequence_GetItem( args, 0 ) :
    args;

  const int n = PySequence_Length( sequence );
  // Prevent empty seguence arguments groups, i.e. Group([])
  if ( n == 0 ){
    PyErr_SetString( PyExc_TypeError, "A group must contain at least one object." );
    return nullptr;
  }

  objects_t faintObjects;
  for ( int i = 0; i != n; i++ ){
    PyObject* object = PySequence_GetItem( sequence, i );
    if ( !PyObject_IsInstance( object, (PyObject*)&SmthType ) ){
      PyErr_SetString( PyExc_TypeError, "Unsupported item in list" );
      return nullptr;
    }
    faintObjects.push_back( ((smthObject*)object)->obj );
  }

  cmd_and_group_t cmd = group_objects_command( faintObjects, select_added(false) );
  faint::python_run_command( self->canvas, cmd.first, self->canvas->GetImage().GetId() );
  return pythoned( cmd.second, self->canvas, self->canvas->GetImage().GetId() );
}

static PyObject* canvas_obj_line(canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateLine>(self,args);
}

static PyObject* canvas_obj_path( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreatePath>(self,args);
}

static PyObject* canvas_obj_polygon( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreatePolygon>(self, args);
}

static PyObject* canvas_obj_raster( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateRaster>(self,args);
}

static PyObject* canvas_obj_rect(canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateRectangle>(self,args);
}

static PyObject* canvas_obj_spline( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateSpline>(self,args);
}

static PyObject* canvas_obj_text( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateText>(self, args);
}

static PyObject* canvas_obj_tri(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Tri tri;
  if ( !parse_tri(args, &tri) ){
    return nullptr;
  }

  Settings settings( default_line_settings() );
  settings.Update( GetAppContext().GetToolSettings() );
  ObjTri* objTri = new ObjTri( tri, settings );
  faint::python_run_command( self->canvas, get_add_object_command(objTri, select_added(false)) );
  return pythoned( objTri, self->canvas, self->canvas->GetImage().GetId() );
}

bool canvas_ok( const CanvasId& c ){
  return GetAppContext().Exists( c );
}

static PyObject* canvas_paste(canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  IntPoint pos;
  if ( !parse_intpoint(args, &pos) ){
    return nullptr;
  }
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    PyErr_SetString(PyExc_OSError, "Failed opening clipboard.");
    return nullptr;
  }

  faint::Bitmap bmp;
  if ( !clipboard.GetBitmap(bmp) ){
    PyErr_SetString(PyExc_OSError, "No bitmap in clipboard.");
    return nullptr;
  }
  const RasterSelection& oldSelection(self->canvas->GetRasterSelection());
  const Settings s(GetAppContext().GetToolSettings());
  faint::python_run_command( self->canvas,
    get_paste_raster_bitmap_command( bmp, pos, oldSelection, s) );
  return Py_BuildValue("");
}

static PyObject* canvas_rect(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Rect r;
  if ( !parse_rect( args, &r ) ){
    PyErr_SetString( PyExc_TypeError, "Invalid parameters to rect(x, y, w, h)" );
    return nullptr;
  }

  Settings s(default_rectangle_settings());
  s.Update( GetAppContext().GetToolSettings() );
  s.Set( ts_AntiAlias, false );
  Command* cmd = new DrawObjectCommand(its_yours(new ObjRectangle(tri_from_rect(r), s)));
  faint::python_run_command(self->canvas, cmd);
  return Py_BuildValue("");
}

static PyObject* canvas_redo(canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->Redo();
  return Py_BuildValue("");
}

PythonErrorReturn canvas_removed_error(){
  PyErr_SetString( PyExc_ValueError, "Operation attempted on closed canvas." );
  return PythonErrorReturn();
}

PyObject* canvas_replace_alpha( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::Color bgColor;
  if ( !parse_rgb_color( args, bgColor ) ){
    return nullptr;
  }
  faint::python_run_command( self->canvas, get_blend_alpha_command(bgColor) );
  return Py_BuildValue("");
}

PyObject* canvas_replace_color( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  if ( PySequence_Length( args ) != 2 ){
    PyErr_SetString( PyExc_TypeError, "Invalid number of arguments");
  }

  faint::Color color1, color2;
  bool colorsOk = parse_color( PySequence_GetItem( args, 0 ), color1 ) &&
    parse_color( PySequence_GetItem( args, 1 ), color2 );

  if ( !colorsOk ){
    return nullptr;
  }

  faint::python_run_command( self->canvas,
    get_replace_color_command(Old(color1), faint::DrawSource(color2)));
  return Py_BuildValue("");
}

static PyObject* canvas_save(canvasObject* self, PyObject* args ){
  using faint::FilePath;
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  if ( PySequence_Length( args ) == 0 ){
    // No filename specified, use the current filename
    Optional<FilePath> maybePath(self->canvas->GetFilePath());
    if ( maybePath.NotSet() ){
      PyErr_SetString( PyExc_ValueError, "No filename specified, and no previously used filename available." );
      return nullptr;
    }

    FilePath filePath(maybePath.Get());
    SaveResult result = faint::Save( *self->canvas, filePath );
    if ( result.Successful() ){
      self->canvas->NotifySaved(filePath);
      return Py_BuildValue("");
    }
    else {
      PyErr_SetString( PyExc_ValueError, result.ErrorDescription().c_str() );
      return nullptr;
    }
  }

  // Use the filename passed as argument
  char* filename = nullptr;
  if ( !PyArg_ParseTuple( args, "s", &filename ) ){
    return nullptr;
  }
  faint::utf8_string fn_u8 = faint::utf8_string(std::string(filename));
  if ( !py_valid_save_filename(fn_u8) ){
    return nullptr;
  }

  FilePath filePath(FilePath::FromAbsolute(fn_u8));
  SaveResult result = faint::Save( *self->canvas, filePath );
  if ( result.Successful() ){
    self->canvas->NotifySaved( filePath );
    return Py_BuildValue("");
  }
  else {
      PyErr_SetString( PyExc_ValueError, result.ErrorDescription().c_str() );
      return nullptr;
  }
}

static PyObject* canvas_save_backup( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  if ( PySequence_Length( args ) == 0 ){
    PyErr_SetString( PyExc_ValueError, "No backup filename specified" );
    return nullptr;
  }

  char* filename = nullptr;
  if ( !PyArg_ParseTuple( args, "s", &filename ) ){
    return nullptr;
  }
  faint::utf8_string fn_u8 = faint::utf8_string(std::string(filename));
  if ( !py_valid_save_filename(fn_u8) ){
    return nullptr;
  }
  faint::FilePath filePath( faint::FilePath::FromAbsolute(fn_u8) );
  SaveResult result = faint::Save( *self->canvas, filePath );
  if ( result.Successful() ){
    return Py_BuildValue("");
  }
  else {
    PyErr_SetString( PyExc_ValueError, result.ErrorDescription().c_str() );
    return nullptr;
  }
}

static PyObject* canvas_scroll_page_left( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageLeft();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_page_right( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageRight();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_page_up( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageUp();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_page_down( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageDown();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_up( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxUp();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_down( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxDown();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_left( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxLeft();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_right( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxRight();
  return Py_BuildValue("");
}

static PyObject* canvas_set_alpha( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  int alpha;
  if ( !PyArg_ParseTuple(args,"i", &alpha) ){
    return nullptr;
  }
  if ( alpha < 0 || 255 < alpha ){
    PyErr_SetString(PyExc_ValueError, "Alpha out of range 0-255");
    return nullptr;
  }
  faint::python_run_command(self->canvas, get_set_alpha_command(static_cast<faint::uchar>(alpha)));
  return Py_BuildValue("");
}

static PyObject* canvas_select_object( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  int n = PySequence_Length( args );
  if ( n == 0 ){
    self->canvas->DeselectObjects();
    GetAppContext().PythonQueueRefresh(self->canvas);
    return Py_BuildValue("");
  }

  PyObject* sequence = args;
  if ( n == 1 ){
    PyObject* first = PySequence_GetItem( sequence, 0 );
    if ( PySequence_Check( first ) ){
      // Unwrapped the single sequence argument (allows select([o1,o2]))
      sequence = first;
      n = PySequence_Length( sequence );
    }
  }

  std::vector<Object*> objects;
  for ( int i = 0; i != n; i++ ){
    PyObject* item = PySequence_GetItem( sequence, i );
    smthObject* obj = as_smthObject( item );
    if ( obj == nullptr ){
      PyErr_SetString(PyExc_TypeError, "That's not a Faint object.");
      return nullptr;
    }
    if ( !self->canvas->Has( obj->objectId ) ){
      PyErr_SetString(PyExc_ValueError, "The canvas does not contain that item.");
      return nullptr;
    }
    objects.push_back(obj->obj);
  }
  self->canvas->SelectObjects( objects, deselect_old(true) );
  return Py_BuildValue("");
}

int canvas_set_grid( canvasObject* self, PyObject* arg, void* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  if ( !PyObject_IsInstance( arg, (PyObject*)&GridType) ){
    PyErr_SetString(PyExc_TypeError, "Argument must be a Grid");
    return faint::setter_fail;
  }

  gridObject* pyGrid = (gridObject*)arg;
  Grid grid = get_grid(pyGrid);
  self->canvas->SetGrid(grid);
  faint::python_queue_refresh(self->canvas);
  return faint::setter_ok;
}

static PyObject* canvas_set_pixel( canvasObject* self, PyObject* args ){
  int x, y, r, g, b, a;
  if ( !PyArg_ParseTuple( args, "ii(iiii)", &x, &y, &r, &g, &b, &a ) ){
    PyErr_Clear();
    a = 255;
    if ( !PyArg_ParseTuple( args, "ii(iii)", &x, &y, &r, &g, &b ) ){
      return nullptr;
    }
  }
  if ( invalid_color(r,g,b,a) ){
    return nullptr;
  }

  const faint::Bitmap& bmp(self->canvas->GetBitmap());
  if ( invalid_pixel_pos(x, y, bmp) ){
    return nullptr;
  }

  faint::python_run_command(self->canvas, get_put_pixel_command(IntPoint(x,y), faint::color_from_ints(r,g,b,a)));
  return Py_BuildValue("");
}

static PyObject* canvas_set_point_overlay(canvasObject* self, PyObject* args){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  IntPoint p;
  if ( !parse_intpoint(args, &p) ){
    return nullptr;
  }
  self->canvas->SetPointOverlay(p);
  faint::python_queue_refresh(self->canvas);
  return Py_BuildValue("");
}

static PyObject* canvas_set_rect( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  int x, y, w, h;
  // Try parsing as normal xywh parameters
  if ( !PyArg_ParseTuple( args, "iiii", &x, &y, &w, &h ) ){
    PyErr_Clear();
    // Try parsing as a single tuple
    if ( !PyArg_ParseTuple( args, "(iiii)", &x, &y, &w, &h ) ){
      return nullptr;
    }
  }

  faint::DrawSource bg( GetAppContext().GetToolSettings().Get( ts_BgCol ) );
  Command* resize = new ResizeCommand( IntRect( IntPoint(x, y), IntSize(w, h) ), bg  );
  faint::python_run_command(self->canvas, resize);
  return Py_BuildValue("");
}

PyObject* canvas_set_scroll_pos( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  int x, y;
  if ( !PyArg_ParseTuple( args, "ii", &x, &y ) ){
    return nullptr;
  }

  self->canvas->SetScrollPos(IntPoint(x,y));
  return Py_BuildValue("");
}

PyObject* canvas_set_selection( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  int x, y, w, h;
  // Try parsing as normal xywh parameters
  if ( !PyArg_ParseTuple( args, "iiii", &x, &y, &w, &h ) ){
    PyErr_Clear();
    // Try parsing as a single tuple
    if ( !PyArg_ParseTuple( args, "(iiii)", &x, &y, &w, &h ) ){
      return nullptr;
    }
  }

  if ( x < 0 || y < 0 ){
    PyErr_SetString( PyExc_ValueError, "The x and y coordinates of the selection rectangle must be positive." );
    return nullptr;
  }
  if ( w <= 0 || h <= 0 ){
    PyErr_SetString( PyExc_ValueError, "The width and height of the selection rectangle must be greater than zero.");
    return nullptr;
  }

  const RasterSelection& currentSelection(self->canvas->GetRasterSelection());
  faint::python_run_command( self->canvas,
    get_selection_rectangle_command( IntRect(IntPoint(x,y), IntSize(w,h)),
      currentSelection ) );
  return Py_BuildValue("");
}

PyObject* canvas_shrink_selection( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  const RasterSelection& selection(self->canvas->GetRasterSelection());
  IntRect selectionRect(selection.GetRect());
  if ( empty( selectionRect ) ){
    IntRect cropRect;
    if ( get_auto_crop_rect( self->canvas->GetBitmap(), cropRect ) ){
      faint::python_run_command( self->canvas,
        get_selection_rectangle_command( translated(cropRect, selectionRect.TopLeft()),
          selection) );
    }
  }
  else {
    faint::Bitmap bmpSelected( sub_bitmap( self->canvas->GetBitmap(), selectionRect ) );
    IntRect cropRect;
    if ( get_auto_crop_rect( bmpSelected, cropRect ) ){
      faint::python_run_command( self->canvas,
        get_selection_rectangle_command( translated(cropRect, selectionRect.TopLeft()), selection ) );
    }
  }
  return Py_BuildValue("");
}

static PyObject* canvas_set_size( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  const int n = PySequence_Length( args );
  if ( n < 3 ){
    // No color specified
    int w = 0;
    int h = 0;
    if ( !PyArg_ParseTuple( args, "ii", &w, &h ) ){
      PyErr_SetString(PyExc_TypeError, "Two numeric arguments required");
      return nullptr;
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString(PyExc_ValueError, "Negative or zero width or height argument");
      return nullptr;
    }
    faint::DrawSource bg = GetAppContext().GetToolSettings().Get( ts_BgCol );
    faint::python_run_command( self->canvas, new ResizeCommand( rect_from_size(IntSize(w, h)), bg ) );
    return Py_BuildValue("");
  }

  int w = 0;
  int h = 0;
  int r = 0;
  int g = 0;
  int b = 0;
  int a = 0;

  if ( !PyArg_ParseTuple( args, "ii(iiii)", &w, &h, &r, &g, &b, &a ) ){
    PyErr_Clear();
    if ( !PyArg_ParseTuple( args, "ii(iii)", &w, &h, &r, &g, &b ) ){
      PyErr_SetString(PyExc_TypeError, "Two numeric arguments and and an RGB- or RGBA-color tuple is required.");
      return nullptr;
    }
    a = 255;
  }

  if ( w <= 0 || h <= 0 ){
    PyErr_SetString(PyExc_ValueError, "Negative or zero width or height argument.");
    return nullptr;
  }

  if ( invalid_color( r, g, b, a ) ){
    return nullptr;
  }

  faint::python_run_command( self->canvas,
    new ResizeCommand( rect_from_size(IntSize(w, h)), faint::DrawSource(faint::color_from_ints(r,g,b,a) )));
  return Py_BuildValue("");
}

static PyObject* canvas_undo(canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->Undo();
  return Py_BuildValue( "" );
}

static PyObject* canvas_zoom_default( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ZoomDefault();
  return Py_BuildValue("");
}

static PyObject* canvas_zoom_fit( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ZoomFit();
  return Py_BuildValue("");
}

static PyObject* canvas_zoom_in( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ZoomIn();
  return Py_BuildValue("");
}

static PyObject* canvas_zoom_out( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ZoomOut();
  return Py_BuildValue("");
}

static PyGetSetDef canvas_getseters[] = {
  {(char*)"grid",
   (getter)canvas_get_grid,
   (setter)canvas_set_grid,
   (char*) "Canvas grid", nullptr},
  {0,0,0,0,0} // Sentinel
};

static PyObject* canvas_brightness( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  const faint::Bitmap& bmp( self->canvas->GetBitmap() );
  const int w = bmp.m_w;
  const int h = bmp.m_h;
  double L = 0;
  for ( int x = 0; x != w; x++ ){
    for ( int y = 0; y != h; y++ ){
      faint::HSL hsl(faint::to_hsl( faint::get_color_raw(bmp, x,y).GetRGB()));
      L += hsl.l;
    }
  }
  L /= w * h;
  return Py_BuildValue("f", L);
}

static PyObject* canvas_brightness_contrast( canvasObject* self, PyObject* args ){
  if (!canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::brightness_contrast_t values;
  if ( !PyArg_ParseTuple( args, "dd", &values.brightness, &values.contrast) ){
    return nullptr;
  }
  faint::python_run_command(self->canvas, get_brightness_and_contrast_command(values));
  return Py_BuildValue("");
}

static PyObject* canvas_pixelize( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  int width;
  if ( !PyArg_ParseTuple( args, "i", &width ) ){
    return nullptr;
  }
  if ( width < 1 ){
    PyErr_SetString(PyExc_ValueError, "Width must be >= 1");
    return nullptr;
  }

  faint::python_run_command(self->canvas, get_pixelize_command(width));
  return Py_BuildValue("");
}

static PyObject* canvas_threshold( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  int min, max;
  if ( !PyArg_ParseTuple( args, "ii", &min, &max ) ){
    return nullptr;
  }

  Interval interval(make_interval(min, max));
  using faint::threshold_range_t;
  if ( !threshold_range_t::Valid(interval) ){
    ClosedIntRange range(as_closed_range<threshold_range_t>());
    PyErr_SetString(PyExc_ValueError, space_sep("Threshold of", str_interval(interval),
	"outside valid range", str_range(range)).c_str());
    return nullptr;
  }
  faint::python_run_command(self->canvas, get_threshold_command(faint::threshold_range_t(interval)));
  return Py_BuildValue("");
}

// Method table for Python-Canvas interface (excluding standard
// functions like del, repr and str) Mind the per-section alphabetical
// ordering!
static PyMethodDef canvas_methods[] = {
  // Raster drawing methods
  {"aa_line", (PyCFunction)canvas_aa_line, METH_VARARGS,
   "aa_line(x0,y0,x1,y1)\nExperimental!\nDraw an anti-aliased line from x0,y0 to x1,y1"},
  {"line", (PyCFunction)canvas_line, METH_VARARGS,
   "line(x0,y0,x1,y1)\nDraw a line from x0,y0 to x1,y1"},
  {"rect", (PyCFunction)canvas_rect, METH_VARARGS,
   "rect(x,y,width,height)\nDraw a rectangle"},
  {"ellipse", (PyCFunction)canvas_ellipse, METH_VARARGS,
   "ellipse(x,y,width,height)\nDraw an ellipse"},

  // Add-object methods
  // Fixme: Argument documentation
  {"Ellipse", (PyCFunction)canvas_obj_ellipse, METH_VARARGS, "Adds an Ellipse object"},
  {"Group", (PyCFunction)canvas_obj_group, METH_VARARGS, "Adds a Group"},
  {"Line", (PyCFunction)canvas_obj_line, METH_VARARGS, "Adds a Line object"},
  {"Path", (PyCFunction)canvas_obj_path, METH_VARARGS, "Adds a Path object"},
  {"Polygon", (PyCFunction)canvas_obj_polygon, METH_VARARGS, "Adds a Polygon object"},
  {"Raster", (PyCFunction)canvas_obj_raster, METH_VARARGS, "Adds a Raster object"},
  {"Rect", (PyCFunction)canvas_obj_rect, METH_VARARGS, "Adds a Rectangle object"},
  {"ObjTri", (PyCFunction)canvas_obj_tri, METH_VARARGS, "Adds a Tri (debug) object"},
  {"Spline", (PyCFunction)canvas_obj_spline, METH_VARARGS, "Adds a Spline object"},
  {"Text", (PyCFunction)canvas_obj_text, METH_VARARGS, "Text((x,y,w,h),s, settings)->Text\nAdds a Text object"},

  // Misc. methods
  {"add_frame", (PyCFunction)canvas_frame_add, METH_VARARGS,
   "Adds a frame to the image"},
  {"auto_crop", (PyCFunction)canvas_auto_crop, METH_NOARGS,
   "auto_crop()\nAutocrops the image."},
  {"blit", (PyCFunction)canvas_blit, METH_VARARGS,
   "blit(x,y, bmp)\nBlits the Bitmap-object at x,y."},
  {"blur", (PyCFunction)canvas_blur, METH_NOARGS,
   "blur()\nBlurs the image."},
  {"brightness", (PyCFunction)canvas_brightness, METH_VARARGS,
   "brightness()\nThe average brightness of the image (the mean sum of the HSL-lightness)"},
  {"brightness_contrast", (PyCFunction)canvas_brightness_contrast, METH_VARARGS,
   "brightness_contrast(brightness,contrast)\nApplies brightness and contrast (also known as bias and gain)."},
  {"center", (PyCFunction)canvas_center_view, METH_VARARGS,
   "center(x,y)\nCenters the view at image position (x,y)"},
  {"close", (PyCFunction)canvas_close, METH_NOARGS,
   "close()\nClose the canvas"},
  {"context_crop", (PyCFunction)canvas_context_crop, METH_NOARGS,
   "context_crop()\nCrops the image to the raster selection, crops a selected object or performs an autocrop."},
  {"context_delete", (PyCFunction)canvas_context_delete, METH_NOARGS,
   "context_delete()\nDelete the selected object or raster region"},
  {"context_flip_horizontal", (PyCFunction)canvas_context_flip_horizontal, METH_NOARGS,
   "context_flip_horizontal()\nFlips the image or selection horizontally"},

   {"context_offset", (PyCFunction)canvas_context_offset, METH_VARARGS,
    "context_offset(dx, dy)\nOffsets the selected objects or raster selection by dx, dy. Scrolls the image if no selection is available."},

  {"context_flip_vertical", (PyCFunction)canvas_context_flip_vertical, METH_NOARGS,
   "context_flip_vertical()\nFlips the image or selection vertically"},
  {"context_rotate_90CW", (PyCFunction)canvas_context_rotate90cw, METH_NOARGS,
   "context_rotate_90CW()\nRotates the image or canvas 90 degrees clock-wise"},
  {"rotate", (PyCFunction)canvas_rotate, METH_VARARGS,
   "rotate(a)\nRotate the image a-radians"},
  {"clear_point_overlay", (PyCFunction)canvas_clear_point_overlay, METH_NOARGS,
   "clear_point_overlay()\n->Clear indicated point (see set_point_overlay)."},
  {"context_flatten", (PyCFunction)canvas_context_flatten, METH_NOARGS,
   "context_flatten()\nFlattens all objects or all selected objects into the background (rasterize)."},
  {"desaturate", (PyCFunction)canvas_desaturate, METH_NOARGS,
   "desaturate()\nDesaturate the image"},
  {"desaturate_weighted", (PyCFunction)canvas_desaturate_weighted, METH_NOARGS,
   "desaturate_weighted()\nDesaturate the image with weighted intensity"},
  {"copy_rect", (PyCFunction)canvas_copy_rect, METH_VARARGS,
   "copy_rect(x,y,w,h)\nCopies the specified rectangle to the clipboard"},
  {"delete_objects", (PyCFunction)canvas_delete_objects, METH_VARARGS,
   "delete_objects(objects)\nDeletes the objects in the sequence from the image"},
  {"erase_but_color", (PyCFunction)canvas_erase_but_color, METH_VARARGS,
   "erase_but_color(keptColor[,eraseColor])\nReplaces all colors, except keptColor, with eraseColor. Uses the current secondary color if keptColor is omitted."},
  {"flatten", (PyCFunction)canvas_flatten, METH_VARARGS,
   "flatten(object1[, object2, ...])\nFlattens the specified objects."},
  {"get_bitmap", (PyCFunction)canvas_get_bitmap, METH_NOARGS,
   "get_bitmap()->bmp\nReturns a copy of the pixel data in the active frame as an ifaint.Bitmap"},
  {"get_frames", (PyCFunction)canvas_get_frames, METH_NOARGS,
   "get_frames() -> [frame1, frame2,...]\nReturns the frames in this image."},
  {"get_frame", (PyCFunction)canvas_get_frame, METH_VARARGS,
   "get_frame([index]) -> Frame\nReturns the frame with the specified index, or the selected frame if no index specified."},
  {"get_frame_delay", (PyCFunction)canvas_frame_get_delay, METH_VARARGS,
   "get_frame_delay(index) -> delay\nReturns the delay for the specified frame."},
  {"get_frame_hotspot", (PyCFunction)canvas_frame_get_hotspot, METH_VARARGS,
   "get_frame_hotspot(index) -> delay\nReturns the hotspot for the specified frame."},
  {"get_filename", (PyCFunction)canvas_filename, METH_NOARGS,
   "get_filename() -> filename\nReturns the filename, if the image has been saved."},
  {"get_id", (PyCFunction)canvas_get_id, METH_NOARGS,
   "get_id() -> canvas_id\nReturns the id that identifies the canvas in this Faint session"},
  {"get_objects", (PyCFunction)canvas_get_objects, METH_NOARGS, "get_objects() -> [ object1, object2, ... ]\nReturns the objects in the active frame of the image"},
  {"get_mouse_pos", (PyCFunction)canvas_get_mouse_pos, METH_NOARGS,
   "get_mouse_pos() -> (x,y)\nReturns the mouse position relative to the image"},
  {"get_scroll_pos", (PyCFunction)canvas_get_scroll_pos, METH_VARARGS,
   "get_scroll_pos() -> x, y\nReturns the scroll bar positions"},
  {"get_selection", (PyCFunction)canvas_get_selection, METH_NOARGS,
   "get_selection() -> (x,y,w,h)\nReturns the selection rectangle or None"},
  {"get_selected", (PyCFunction)canvas_get_selected_objects, METH_NOARGS, "get_sel_obj() -> [ object1, object2, ... ]\nReturns a list of the currently selected objects"},
  {"get_zoom", (PyCFunction)canvas_get_zoom, METH_NOARGS,
   "get_zoom() -> zoom\nReturns the current zoom as a floating point value"},
  {"paste", (PyCFunction)canvas_paste, METH_VARARGS,
   "paste(x,y)\nPastes a bitmap from the clipboard to (x,y)"},
  {"set_point_overlay", (PyCFunction)canvas_set_point_overlay, METH_VARARGS,
   "set_point_overlay(x,y)\n->Indicate a point."},
  {"invert", (PyCFunction)canvas_invert, METH_NOARGS,
   "invert()\nInvert the colors of the image"},
  {"get_colors", (PyCFunction)canvas_get_color_list, METH_NOARGS,
   "get_colors()->[c1, c2, ..., cn]\nReturns a list of the unique colors used in the image."},
  {"get_size", (PyCFunction)canvas_get_size, METH_NOARGS,
   "get_size() -> (w,h)\nReturns the size of the image."},
  {"next_frame", (PyCFunction)canvas_frame_next, METH_NOARGS,
   "next_frame()\nSelects the next frame"},
  {"prev_frame", (PyCFunction)canvas_frame_previous, METH_NOARGS,
   "prev_frame()\nSelects the previous frame"},
  {"redo", (PyCFunction)canvas_redo, METH_NOARGS,
   "redo()\nRedo the last undone action"},
  {"replace_color", (PyCFunction)canvas_replace_color, METH_VARARGS,
   "replace_color(old, new)\nReplaces all pixels matching the color old with the color new.\nThe colors are specified as r,g,b[,a]-tuples"},
  {"replace_alpha", (PyCFunction)canvas_replace_alpha, METH_VARARGS,
   "replace_alpha(r,g,b)\nBlends all pixels with alpha towards the specified color"},
  {"save", (PyCFunction)canvas_save, METH_VARARGS,
   "save(filename)\nSave the image to file"},
  {"save_backup", (PyCFunction)canvas_save_backup, METH_VARARGS,
   "save_backup(filename)\nSave a copy of the image to the specified file without changing the target filename for the image"},
  {"select", (PyCFunction)canvas_select_object, METH_VARARGS,
   "select(object(s))\nSelects the object or list of objects specified.\nThe previous selection will be discarded"},
  {"deselect", (PyCFunction)canvas_deselect_object, METH_VARARGS,
   "deselect(object(s))\nDeselects the object or list of objects."},
  {"set_alpha", (PyCFunction)canvas_set_alpha, METH_VARARGS,
   "set_alpha(a)\nSets the alpha component of all pixels to a"},
  {"set_pixel", (PyCFunction)canvas_set_pixel, METH_VARARGS,
   "set_pixel(x,y,(r,g,b[,a]))\nSets the pixel x,y to the specified color"},
  {"get_max_scroll", (PyCFunction)canvas_get_max_scroll, METH_VARARGS,
   "get_max_scroll() -> x,y\nReturns the largest useful scroll positions"},
  {"get_pixel", (PyCFunction)canvas_get_pixel, METH_VARARGS,
   "get_pixel(x,y) -> (r,g,b,a)\nReturns the color at x,y"},
  {"get_background_png_string", FAINT_KW_TO_PY(canvas_get_background_png_string), METH_KEYWORDS,
   "get_background_png_string(stamp=True|False) -> s\nReturns the background encoded in png as a string.\nIf the keyword argument stamp is True, any floating raster selection will be stamped onto the image."},
  {"pixelize", (PyCFunction)canvas_pixelize, METH_VARARGS,
   "pixelize(width)\nPixelize the image with width"},
  {"set_frame_delay", (PyCFunction)canvas_frame_set_delay, METH_VARARGS,
   "set_frame_delay( frame, ms )\nSets the delay in milliseconds for the specified frame"},
  {"set_frame_hotspot", (PyCFunction)canvas_frame_set_hotspot, METH_VARARGS,
   "set_frame_hotspot(frame,(x,y))\nSets the hotspot for the specified frame"},
  {"set_rect", (PyCFunction)canvas_set_rect, METH_VARARGS,
   "set_rect(x,y,w,h)\nSets the image size to w,h extending from x,y.\nx and y may be negative"},
  {"set_scroll_pos", (PyCFunction)canvas_set_scroll_pos, METH_VARARGS,
   "set_scroll_pos(x, y)\nSets the scroll bar positions"},
  {"set_selection", (PyCFunction)canvas_set_selection, METH_VARARGS,
   "set_selection(x,y,w,h)\nSets the selection rectangle"},
  {"shrink_selection", (PyCFunction)canvas_shrink_selection, METH_VARARGS,
   "shrink_selection()\nAuto-shrink the selection rectangle to an image detail by trimming same-colored areas from its sides"},
  {"set_size", (PyCFunction)canvas_set_size, METH_VARARGS,
   "set_size(w,h)\nSets the image size to w,h"},
  {"set_threshold", (PyCFunction)canvas_threshold, METH_VARARGS,
   "set_threshold(min, max)\nBinarize the image, setting pixels within the thresholds white, the other black"}, // Fixme: document ranges
  {"undo", (PyCFunction)canvas_undo, METH_NOARGS,
   "undo()\nUndo the last action"},
  {"scroll_page_left", (PyCFunction)canvas_scroll_page_left, METH_NOARGS,
   "scroll_page_left()\nScroll the image one page to the left"},
  {"scroll_page_right", (PyCFunction)canvas_scroll_page_right, METH_NOARGS,
   "scroll_page_right()\nScroll the image one page to the right"},
  {"scroll_page_up", (PyCFunction)canvas_scroll_page_up, METH_NOARGS,
   "scroll_page_up()\nScroll the image one page up"},
  {"scroll_page_down", (PyCFunction)canvas_scroll_page_down, METH_NOARGS,
   "scroll_page_down()\nScroll the image one page down"},
  {"scroll_max_left", (PyCFunction)canvas_scroll_max_left, METH_NOARGS,
   "scroll_max_left()\nScroll the image to the left"},
  {"scroll_max_right", (PyCFunction)canvas_scroll_max_right, METH_NOARGS,
   "scroll_max_right()\nScroll the image to the right"},
  {"scroll_max_up", (PyCFunction)canvas_scroll_max_up, METH_NOARGS,
   "scroll_max_up()\nScroll the image up"},
  {"scroll_max_down", (PyCFunction)canvas_scroll_max_down, METH_NOARGS,
   "scroll_max_down()\nScroll the image to the bottom"},
  {"dwim", (PyCFunction)canvas_dwim, METH_NOARGS,
   "dwim()\nAlternate the behavior of the last run command"},
  {"zoom_default", (PyCFunction)canvas_zoom_default, METH_NOARGS,
   "zoom_default()\nSet zoom to 1:1"},
  {"zoom_fit", (PyCFunction)canvas_zoom_fit, METH_NOARGS,
   "zoom_fit()\nZoom image to fit view"},
  {"zoom_in", (PyCFunction)canvas_zoom_in, METH_NOARGS,
   "zoom_in()\nZoom in one step"},
  {"zoom_out", (PyCFunction)canvas_zoom_out, METH_NOARGS,
   "zoom_out()\nZoom out one step"},
  {0,0,0,0}  // Sentinel
};

// Python standard methods follow...
static PyObject* canvas_new(PyTypeObject* type, PyObject*, PyObject*){
  canvasObject *self;
  self = (canvasObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* canvas_repr(canvasObject* self ){
  std::stringstream ss;
  if ( canvas_ok( self->id ) ){
    ss << "Canvas #" << self->id.Raw();
    Optional<faint::FilePath> filePath( self->canvas->GetFilePath() );
    if ( filePath.IsSet() ){
      ss << " " << filePath.Get().ToAscii(); // Fixme: Truncates wide filenames
    }
  }
  else{
    ss << "Retired Canvas #" << self->id.Raw();
  }
  return Py_BuildValue("s", ss.str().c_str());
}

long canvas_hash(PyObject* selfRaw ){
  canvasObject* self((canvasObject*)selfRaw);
  return self->canvas->GetId().Raw();
}

int canvas_compare(canvasObject* self, PyObject* otherRaw){
  if ( !PyObject_IsInstance( otherRaw, (PyObject*)&CanvasType) ){
    // Use less for type mismatch
    return faint::cmp_less;
  }
  canvasObject* other((canvasObject*)otherRaw);
  return faint::py_compare(self->canvas->GetId(), other->canvas->GetId());
}

std::string TranslatePath( const std::string& path ){
  if ( path.size() == 0 ){
    return path;
  }
  if ( path[0] == '~' ){
    return get_home_dir() + path.substr( 1, path.size() - 1 );
  }

  return path;
}

// General canvas init error
static const char* const g_canvasInitError = "Incorrect parameters. Canvas supports: Canvas(width, height)\nCanvas(width, height, (r,g,b[,a]))\nCanvas(filename(s))";

static int canvas_init_from_filename_sequence( canvasObject* self, const std::vector<std::string>& path_vec ){
  faint::FileList filePaths;
  for ( size_t i = 0; i != path_vec.size(); i++ ){
    const faint::utf8_string path(path_vec[i]);
    if ( !faint::is_absolute_path(path) ){
      std::string error("Error: " + path.str() + " is relative");
      PyErr_SetString(PyExc_ValueError, error.c_str());
      return faint::init_fail;
    }
    if ( !faint::is_file_path(path) ){
      std::string error("Error: " + path.str() + " is not a valid file path");
      PyErr_SetString(PyExc_ValueError, error.c_str());
      return faint::init_fail;
    }
    filePaths.push_back(faint::FilePath::FromAbsolute(path));
  }

  assert(!filePaths.empty());

  if ( filePaths.size() == 1 ){
    CanvasInterface* canvas = GetAppContext().Load( filePaths.back(), change_tab(true) );
    if ( canvas == nullptr ){
      PyErr_SetString(PyExc_ValueError, "Failed loading"); // Fixme
      return faint::init_fail;
    }
    return faint::init_ok;
  }

  assert(filePaths.size() > 1 );
  CanvasInterface* canvas = GetAppContext().LoadAsFrames(filePaths, change_tab(true));
  if ( canvas == nullptr ){
    PyErr_SetString(PyExc_ValueError, "Failed loading"); // Fixme: Indicated failed filename
    return faint::init_fail;
  }
  self->canvas = canvas;
  self->id = self->canvas->GetId();
  return faint::init_ok;
}

static int canvas_init_no_args( canvasObject* self ){
  AppContext& ctx = GetAppContext();
  self->canvas = &( ctx.NewDocument( ctx.GetDefaultImageInfo() ) );
  self->id = self->canvas->GetId();
  return faint::init_ok;
}

// Canvas-constructor, adds a new canvas to faint
static int canvas_init(canvasObject* self, PyObject* args, PyObject* ){
  const int n = PySequence_Length( args );

  if ( n == 0 ){
    return canvas_init_no_args(self);
  }
  else if ( faint::is_string_sequence(args) ){
    return canvas_init_from_filename_sequence( self, faint::parse_string_sequence(args));
  }
  else if ( n == 2 ){
    int w, h;
    if ( !PyArg_ParseTuple( args, "ii", &w, &h ) ){
      PyErr_SetString( PyExc_TypeError, g_canvasInitError );
      return faint::init_fail;
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString( PyExc_ValueError, "Zero or negative Width or height passed to Canvas constructor");
      return faint::init_fail;
    }
    self->canvas = &( GetAppContext().NewDocument(ImageInfo(IntSize(w, h))));
    self->id = self->canvas->GetId();
    return faint::init_ok;
  }
  else if ( n == 3 ){
    int w, h, r, g, b;
    int a = 255;
    bool rgb = PyArg_ParseTuple( args, "ii(iii)", &w, &h, &r, &g, &b ) != 0;
    if ( !rgb ){
      PyErr_Clear();
      bool rgba = PyArg_ParseTuple( args, "ii(iiii)", &w, &h, &r, &g, &b, &a ) != 0;
      if ( !rgba ){
	PyErr_SetString( PyExc_TypeError, g_canvasInitError );
	return faint::init_fail;
      }
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString( PyExc_ValueError, "Negative width or height passed to Canvas constructor");
      return faint::init_fail;
    }
    if ( invalid_color(r,g,b,a) ){
      return faint::init_fail;
    }
    self->canvas = &( GetAppContext().NewDocument( ImageInfo(IntSize(w,h), faint::color_from_ints(r,g,b,a) ) ) );
    self->id = self->canvas->GetId();
    return faint::init_ok;
  }
  PyErr_SetString( PyExc_TypeError, g_canvasInitError );
  return faint::init_fail;
}

PyTypeObject CanvasType = {
  PyObject_HEAD_INIT(nullptr)
  0, // ob_size
  "Canvas", //tp_name
  sizeof(canvasObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  (cmpfunc)canvas_compare, // tp_compare
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
  0, // tp_richcompare
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
  0 // tp_version_tag
};

PyObject* pythoned( CanvasInterface& canvas ){
  canvasObject* py_canvas = (canvasObject*)CanvasType.tp_alloc( &CanvasType, 0 );
  py_canvas->canvas = &canvas;
  py_canvas->id = py_canvas->canvas->GetId();
  return (PyObject*) py_canvas;
}
