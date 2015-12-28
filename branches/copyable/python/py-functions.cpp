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
#include <iostream> // for print_to_stdout
#include "python/py-include.hh"
#include "app/app.hh" // Fixme: For get_art_container
#include "app/get-app-context.hh"
#include "gui/brightness-contrast-dialog.hh" // Fixme
#include "gui/color-balance-dialog.hh" // Fixme
#include "gui/resize-dialog.hh" // Fixme
#include "gui/rotate-dialog.hh" // Fixme
#include "gui/threshold-dialog.hh" // Fixme
#include "objects/objellipse.hh"
#include "objects/objpath.hh"
#include "objects/objrectangle.hh"
#include "objects/objline.hh"
#include "objects/objraster.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "python/generate/output/interface.hh"
#include "python/py-app-objects.hh"
#include "python/py-canvas.hh"
#include "python/py-format.hh"
#include "python/py-frame.hh"
#include "python/py-functions.hh"
#include "python/py-grid.hh"
#include "python/py-settings.hh"
#include "python/py-something.hh"
#include "python/py-util.hh"
#include "python/py-ugly-forward.hh"
#include "python/py-interface.hh"
#include "util/canvas.hh"
#include "util/clipboard.hh"
#include "util/file-path-util.hh"
#include "util/font.hh"
#include "util/image.hh"
#include "util/keycode.hh"
#include "util/optional.hh"
#include "util/pattern.hh"
#include "util/settings.hh"

namespace faint{

PyObject* add_format(PyObject*, PyObject* args){
  PyObject* loader;
  PyObject* saver;
  char* name;
  char* extension;
  if (!PyArg_ParseTuple(args, "OOss", &loader, &saver, &name, &extension)) {
    return nullptr;
  }

  if (loader == Py_None){
    loader = nullptr;
  }
  else if (!PyCallable_Check(loader)){
    PyErr_SetString(PyExc_TypeError, "Loader must be a callback or None:\nadd_format(loader, saver, name, extension)");
    return nullptr;
  }
  else {
    Py_INCREF(loader);
  }

  if (saver == Py_None){
    saver = nullptr;
  }
  else if (!PyCallable_Check(saver)){
    PyErr_SetString(PyExc_TypeError, "Saver must be a callback or None");
    return nullptr;
  }
  else{
    Py_INCREF(saver);
  }

  if (loader == nullptr && saver == nullptr){
    PyErr_SetString(PyExc_ValueError, "saver and loader both None");
    return nullptr;
  }
  PyFileFormat* f = new PyFileFormat(load_callback_t(loader), save_callback_t(saver), label_t(utf8_string(name)), FileExtension(extension));
  get_app_context().AddFormat(f);
  return Py_BuildValue("");
}

PyObject* autosize_raster(PyObject*, PyObject* args){
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)){
    PyErr_SetString(PyExc_TypeError, "autosize_raster requires a raster object");
    return nullptr;
  }

  std::pair<Canvas*, ObjRaster*> p = as_ObjRaster(obj);
  if (p.second == nullptr){
    PyErr_SetString(PyExc_ValueError, "autosize_raster requires a raster object");
    return nullptr;
  }
  python_run_command(*p.first, crop_raster_object_command(p.second));
  return Py_BuildValue("");
}

PyObject* autosize_text(PyObject*, PyObject* args){
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)){
    PyErr_SetString(PyExc_TypeError, "autosize_text requires a text object");
    return nullptr;  }

  std::pair<Canvas*, ObjText*> p = as_ObjText(obj);
  if (p.second == nullptr){
    PyErr_SetString(PyExc_ValueError, "autosize_text requires a text object");
    return nullptr;
  }
  python_run_command(*p.first, crop_text_region_command(p.second));
  return Py_BuildValue("");
}

static PyObject* copy_text(PyObject*, PyObject* args){
  char* str = nullptr;
  if (!PyArg_ParseTuple(args, "s", &str)){
    return nullptr;
  }

  Clipboard clipboard;
  if (!clipboard.Good()){
    PyErr_SetString(PyExc_OSError, "Failed opening clipboard");
    return nullptr;
  }
  clipboard.SetText(utf8_string(str));
  return Py_BuildValue("");
}

static void dialog_color_balance(){
  AppContext& app = get_app_context();
  app.Modal(show_color_balance_dialog);
  //  app.ModalFull(bind_show_resize_dialog(get_art_container()));
}

static void dialog_resize(){
  AppContext& app = get_app_context();
  // Fixme: get_art_container here is problematic - should be accessed
  //via some context
  app.ModalFull(bind_show_resize_dialog(get_art_container()));
}

static void dialog_rotate(){
  AppContext& app = get_app_context();
  // Fixme: get_art_container here is problematic - should be accessed
  // via some context
  app.ModalFull(bind_show_rotate_dialog(get_art_container()));
}

static void dialog_brightness_contrast(){
  get_app_context().Modal(show_brightness_contrast_dialog);
}

static void dialog_help(){
  get_app_context().ToggleHelpFrame();
}

static void dialog_open_file(){
  get_app_context().DialogOpenFile();
}

static void dialog_threshold(){
  AppContext& app = get_app_context();
  const Settings& s = app.GetToolSettings();
  app.Modal(bind_show_threshold_dialog(s.Get(ts_Fg), s.Get(ts_Bg)));
}

static void dialog_python_console(){
  get_app_context().TogglePythonConsole();
}

static void faint_quit(){
  get_app_context().Quit();
}

Canvas& get_active_canvas(){
  return get_app_context().GetActiveCanvas();
}

static PyObject* get_canvas_list(PyObject*, PyObject*){
  AppContext& app = get_app_context();
  int numCanvas = app.GetCanvasCount();
  PyObject* py_list = PyList_New(0);
  for (int i = 0; i != numCanvas; i++){
    PyList_Append(py_list, pythoned(app.GetCanvas(i)));
  }
  return py_list;
}

static Paint get_bg(){
  return get_app_context().Get(ts_Bg);
}

static Paint get_fg(){
  return get_app_context().Get(ts_Fg);
}

static StrSetting::ValueType get_font(){
  return get_app_context().Get(ts_FontFace);
}

static int get_layer(){
  return static_cast<int>(get_app_context().GetLayerType());
}

static IntPoint get_mouse_pos_screen(){
  return get_app_context().GetMousePos();
}

Settings get_settings(){
  return get_app_context().GetToolSettings();
}

static PyObject* set_active_canvas(PyObject*, PyObject* args){
  PyObject* o = nullptr;
  if (!PyArg_ParseTuple(args, "O", &o)){
    return nullptr;
  }
  if (!PyObject_IsInstance(o, (PyObject*)&CanvasType)){
    PyErr_SetString(PyExc_TypeError, "Argument must be a Canvas");
    return nullptr;
  }

  canvasObject* canvas = (canvasObject*)o;
  get_app_context().SetActiveCanvas(canvas->id);
  return Py_BuildValue("");
}

static void set_bg(const Paint& bg){
  get_app_context().Set(ts_Bg, bg);
}

static void set_fg(const Paint& fg){
  get_app_context().Set(ts_Fg, fg);
}

static void set_layer(const int& layer){
  if (!valid_layerstyle(layer)){
    std::stringstream ss;
    ss << "Invalid value for layer.\nAcceptable values are:\n" <<
      to_int(Layer::RASTER) << " - Raster layer\n" <<
      to_int(Layer::OBJECT) << " - Object layer\n";
    throw ValueError(utf8_string(ss.str()));
  }
  get_app_context().SetLayer((to_layerstyle(layer)));
}

static void incomplete_command(){
  get_app_context().PythonContinuation();
}

static void int_bind_key(const KeyPress& keyPress){
  // Inform the C++ side that the key is bound
  get_app_context().Bind(keyPress, bind_global(false));
}

static void int_bind_key_global(const KeyPress& keyPress){
  // Inform the C++ side that the key is bound globally
  get_app_context().Bind(keyPress, bind_global(true));
}

static void int_faint_print(const utf8_string& s) {
  get_app_context().PythonIntFaintPrint(s);
}

static void int_get_key(){
  get_app_context().PythonGetKey();
}

static utf8_string int_get_key_name(const int& keyCode){
  return KeyPress(Key(keyCode)).Name();
}

static void int_unbind_key(const KeyPress& keyPress){
  // Inform the C++ side that the key is unbound
  get_app_context().Unbind(keyPress);
}

std::vector<std::pair<utf8_string, utf8_string>> list_formats(){
  std::vector<Format*> formats = get_app_context().GetFormats();
  std::vector<std::pair<utf8_string, utf8_string> > formatInfo;
  formatInfo.reserve(formats.size());
  for (Format* f : formats){
    formatInfo.push_back(std::make_pair(f->GetLabel(),
      f->GetDefaultExtension().Str()));
  }
  return formatInfo;
}

void print_to_stdout(const std::string& s){
  std::cout << s << std::endl;
}

static void read_eval_print_done(){
  AppContext& app(get_app_context());
  app.PythonDone();
  app.PythonNewPrompt();
  get_app_context().PythonDone();
}

static void set_font(const utf8_string& fontStr){
  if (!valid_facename(fontStr)){
    throw ValueError("Invalid facename"); // Fixme: Better error
  }
  AppContext& ctx = get_app_context();
  ctx.Set(ts_FontFace, fontStr);
}

static void swap_colors(){
  AppContext& app(get_app_context());
  Paint fg(app.Get(ts_Fg));
  Paint bg(app.Get(ts_Bg));
  app.Set(ts_Fg, bg);
  app.Set(ts_Bg, fg);
}

static Optional<int> tool(const Optional<int>& maybeId){
  if (maybeId.NotSet()){
    return option(to_int(get_app_context().GetToolId()));
  }

  int id = maybeId.Get();
  if (id < to_int(ToolId::MIN_ID) || to_int(ToolId::MAX_VALID_ID) < id){
    std::stringstream ss;
    ss << "Invalid tool identifier. Valid tool identifiers are " << to_int(ToolId::MIN_ID) << " to " << to_int(ToolId::MAX_VALID_ID);
    throw ValueError(utf8_string(ss.str()));
  }

  get_app_context().SelectTool(to_tool_id(id));
  return no_option();
}

static void tool_raster_selection(){
  get_app_context().SelectTool(ToolId::RECTANGLE_SELECTION);
}

static void tool_object_selection(){
  get_app_context().SelectTool(ToolId::OBJECT_SELECTION);
}

static void tool_pen(){
  get_app_context().SelectTool(ToolId::PEN);
}

static void tool_brush(){
  get_app_context().SelectTool(ToolId::BRUSH);
}

static void tool_picker(){
  get_app_context().SelectTool(ToolId::PICKER);
}

static void tool_path(){
  get_app_context().SelectTool(ToolId::PATH);
}

static void tool_line(){
  get_app_context().SelectTool(ToolId::LINE);
}

static void tool_spline(){
  get_app_context().SelectTool(ToolId::SPLINE);
}

static void tool_rectangle(){
  get_app_context().SelectTool(ToolId::RECTANGLE);
}

static void tool_ellipse(){
  get_app_context().SelectTool(ToolId::ELLIPSE);
}

static void tool_polygon(){
  get_app_context().SelectTool(ToolId::POLYGON);
}

static void tool_text(){
  get_app_context().SelectTool(ToolId::TEXT);
}

static void tool_fill(){
  get_app_context().SelectTool(ToolId::FLOOD_FILL);
}

std::string comma_sep(const Point& pt){
  std::stringstream ss;
  ss << pt.x << "," << pt.y;
  return ss.str();
}

static PyObject* to_svg_path(PyObject*, PyObject* args){
  PyObject* pyObj = nullptr;
  if (! PyArg_ParseTuple(args, "O", &pyObj)){
    return nullptr;
  }
  Object* object = as_Object(pyObj);
  if (object == nullptr){
    PyErr_SetString(PyExc_TypeError, "Not a Faint Object"); // Fixme
    return nullptr;
  }

  return build_unicode(points_to_svg_path_string(object->GetPath()));
}

static PyObject* get_path_points(PyObject*, PyObject* args){
  PyObject* pyObj = nullptr;
  if (! PyArg_ParseTuple(args, "O", &pyObj)){
    return nullptr;
  }
  Object* obj = as_Object(pyObj);

  std::vector<PathPt> points(obj->GetPath());
  if (points.empty()){
    PyErr_SetString(PyExc_ValueError, "Empty path");
  }

  const int numPoints = resigned(points.size());
  PyObject* list = PyList_New(numPoints);
  for (int i = 0; i != numPoints; i++){
    const PathPt& pt = points[to_size_t(i)];
    if (pt.IsArc()){
      PyObject* arcTo = PyList_New(8);
      PyList_SetItem(arcTo, 0, PyUnicode_FromString("A"));
      PyList_SetItem(arcTo, 1, PyFloat_FromDouble(pt.p.x));
      PyList_SetItem(arcTo, 2, PyFloat_FromDouble(pt.p.y));
      PyList_SetItem(arcTo, 3, PyFloat_FromDouble(pt.r.x));
      PyList_SetItem(arcTo, 4, PyFloat_FromDouble(pt.r.y));
      PyList_SetItem(arcTo, 5, PyFloat_FromDouble(pt.axis_rotation));
      PyList_SetItem(arcTo, 6, PyLong_FromLong(pt.large_arc_flag));
      PyList_SetItem(arcTo, 7, PyLong_FromLong(pt.sweep_flag));

      PyList_SetItem(list, i, arcTo);
    }
    else if (pt.ClosesPath()){
      PyObject* close = PyList_New(1);
      PyList_SetItem(close, 0, PyUnicode_FromString("Z"));

      PyList_SetItem(list, i, close);
    }
    else if (pt.IsCubicBezier()){
      PyObject* bezier = PyList_New(7);
      PyList_SetItem(bezier, 0, PyUnicode_FromString("C"));
      PyList_SetItem(bezier, 1, PyFloat_FromDouble(pt.p.x));
      PyList_SetItem(bezier, 2, PyFloat_FromDouble(pt.p.y));
      PyList_SetItem(bezier, 3, PyFloat_FromDouble(pt.c.x));
      PyList_SetItem(bezier, 4, PyFloat_FromDouble(pt.c.y));
      PyList_SetItem(bezier, 5, PyFloat_FromDouble(pt.d.x));
      PyList_SetItem(bezier, 6, PyFloat_FromDouble(pt.d.y));

      PyList_SetItem(list, i, bezier);
    }
    else if (pt.IsLine()){
      PyObject* lineTo = PyList_New(3);
      PyList_SetItem(lineTo, 0, PyUnicode_FromString("L"));
      PyList_SetItem(lineTo, 1, PyFloat_FromDouble(pt.p.x));
      PyList_SetItem(lineTo, 2, PyFloat_FromDouble(pt.p.y));

      PyList_SetItem(list, i, lineTo);
    }
    else if (pt.IsMove()){
      PyObject* moveTo = PyList_New(3);
      PyList_SetItem(moveTo, 0, PyUnicode_FromString("M"));
      PyList_SetItem(moveTo, 1, PyFloat_FromDouble(pt.p.x));
      PyList_SetItem(moveTo, 2, PyFloat_FromDouble(pt.p.y));

      PyList_SetItem(list, i, moveTo);
    }
  }
  return list;
}

static void update_settings(const Settings& settings){
  get_app_context().UpdateToolSettings(settings);
}

static PyObject* one_color_bg(PyObject*, PyObject* args){
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)){
    return nullptr;
  }
  if (!PyObject_IsInstance(obj, (PyObject*)&FrameType)){
    PyErr_SetString(PyExc_TypeError, "Object is not a frame");
    return nullptr;
  }

  frameObject* frame = (frameObject*)obj;
  if (!faint_side_ok(frame)){
    return nullptr;
  }
  const Image& image = frame->canvas->GetFrameById(frame->frameId);
  auto& maybeBmp = image.GetBg().Get<Bitmap>();
  return python_bool(maybeBmp.NotSet() || is_blank(maybeBmp.Get()));
}

static PyObject* bitmap_from_png_string(PyObject*, PyObject* args){
  Bitmap bmp;
  if (!parse_png_bitmap(args, bmp)){
    return nullptr;
  }
  return build_bitmap(bmp);
}

static PyObject* get_active_grid(PyObject*, PyObject*){
  return py_grid_canvas(nullptr);
}

std::string get_pattern_status(){
  std::map<int,int> status = pattern_status();
  std::stringstream ss;
  for (const auto& idToCount : status){
    ss << idToCount.first << "=" << idToCount.second << ",";
  }
  return ss.str();
}

static PyMethodDef faint_interface_methods[] = {
   {"add_format", add_format, METH_VARARGS, "add_format(load_callback, save_callback, name, extension)\nAdd a file format for loading and/or saving.\n\nEither load_callback or save_callback can be None.\nname is the name of the file format, extension is the file extension, used to identify the format.\n\nThe load_callback receives a filename and an ImageProps object:\n  my_load_callback(filename, imageProps)\n\nThe save_callback receives a target filename and a Canvas:\n  my_save_callback(filename, canvas)" },
   {"autosize_raster", autosize_raster, METH_VARARGS, "autosize_raster(raster_object)\nTrims away same-colored borders around a raster object"},
   {"autosize_text", autosize_text, METH_VARARGS, "autosize_text(text_object)\nResizes the objects text box to snugly fit the text"},
   {"bitmap_from_png_string", bitmap_from_png_string, METH_VARARGS, "bitmap_from_png_string(s) -> bmp\nCreates a bitmap from the PNG string"},
   {"copy_text", copy_text, METH_VARARGS, "copy_text(s)\nCopies s to clipboard"},
   {"cout", FREE_FORWARDER(print_to_stdout), METH_VARARGS, "Uses std::cout"},
   {"dialog_brightness_contrast", FREE_FORWARDER(dialog_brightness_contrast), METH_NOARGS, "Show the brightness contrast dialog"},
   {"dialog_help", FREE_FORWARDER(dialog_help), METH_NOARGS, "Show the Help dialog"},
   {"dialog_color_balance", FREE_FORWARDER(dialog_color_balance), METH_NOARGS, "Show the color balance dialog"},
   {"dialog_open_file", FREE_FORWARDER(dialog_open_file), METH_NOARGS, "Show the Open file dialog"},
   {"dialog_python_console", FREE_FORWARDER(dialog_python_console), METH_NOARGS, "Show the Python console"},
   {"dialog_resize", FREE_FORWARDER(dialog_resize), METH_NOARGS, "Show the resize dialog"},
   {"dialog_rotate", FREE_FORWARDER(dialog_rotate), METH_NOARGS, "Show the rotate dialog"},
   {"dialog_threshold", FREE_FORWARDER(dialog_threshold), METH_NOARGS, "Show the threshold dialog."},
   {"faint_quit", FREE_FORWARDER(faint_quit), METH_NOARGS, "Exit faint"},
   {"get_active_image", FREE_FORWARDER(get_active_canvas), METH_VARARGS, "get_active_image() -> Canvas \nReturns the active (currently edited) image."},
   {"set_active_image", set_active_canvas, METH_VARARGS, "set_active_image(canvas) -> \nActivates (selects in a tab) the specified image."},
   {"get_path_points", get_path_points, METH_VARARGS, "get_path_points(path_object) -> l\nReturns a list of path points"}, // FIXME
   {"get_config_path", FREE_FORWARDER(get_user_config_file_path), METH_VARARGS, "get_config_path() -> s\nReturns the path to the user's Python configuration file"},
   {"get_config_dir", FREE_FORWARDER(get_home_dir), METH_VARARGS, "get_config_dir() -> s\nReturns the path to the directory with the user's Python configuration file"},
   {"get_bg", FREE_FORWARDER(get_bg), METH_VARARGS, "get_bg()->bg\nGet the Background color, Gradient or Pattern."},
   {"get_fg", FREE_FORWARDER(get_fg), METH_VARARGS, "get_fg()->fg\nReturns the foreground color, Gradient or Pattern."},
   {"get_font", FREE_FORWARDER(get_font), METH_NOARGS, "get_font()\n\nReturns the active font face name."},
   {"get_layer", FREE_FORWARDER(get_layer), METH_NOARGS, "Returns the layer index"},
   {"get_mouse_pos_screen", FREE_FORWARDER(get_mouse_pos_screen), METH_NOARGS, "Returns the mouse pointer position"},
   {"get_settings", FREE_FORWARDER(get_settings), METH_NOARGS, "get_settings() -> Settings \nReturns the current tool settings"},
   {"int_bind_key", FREE_FORWARDER(int_bind_key), METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_bind_key_global", FREE_FORWARDER(int_bind_key_global), METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_faint_print", FREE_FORWARDER(int_faint_print), METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_get_key", FREE_FORWARDER(int_get_key), METH_VARARGS, "Faint framework function, not indended for use."},
   {"int_get_key_name", FREE_FORWARDER(int_get_key_name), METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_incomplete_command", FREE_FORWARDER(incomplete_command), METH_VARARGS, "Faint framework function, not intended for use"},
   {"int_ran_command", FREE_FORWARDER(read_eval_print_done), METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_unbind_key", FREE_FORWARDER(int_unbind_key), METH_VARARGS, "Faint framework function, not intended for use."},
   {"list_fonts", FREE_FORWARDER(available_font_facenames), METH_NOARGS, "list_fonts() -> list of fonts\n\nReturns a list with the names of all available fonts."},
   {"list_formats", FREE_FORWARDER(list_formats), METH_NOARGS, "List available file formats."},
   {"list_images", get_canvas_list, METH_VARARGS, "list_images() -> [c1, c2, ...]\nRetrieve a list of all opened images as Canvas-objects."},
   {"one_color_bg", one_color_bg, METH_VARARGS, "one_color_bg(frame) -> b\nTrue if the frame background consists of a single color\n"},
   {"pattern_status", FREE_FORWARDER(get_pattern_status), METH_VARARGS, "Fixme"},
   {"set_bg", FREE_FORWARDER(set_bg), METH_VARARGS,
    "set_bg(c)\n\nSet the background color. Accepts a color tuple, a Gradient or a Pattern."},
   {"set_fg", FREE_FORWARDER(set_fg), METH_VARARGS,
    "set_fg(r,g,b,[a]) Set the foreground color. Accepts a color tuple, a Gradient or a Pattern."},
   {"set_font", FREE_FORWARDER(set_font), METH_VARARGS, "set_font(fontname)\n\nSet the active font. Takes one string parameter, the font face name."},
   {"set_layer", FREE_FORWARDER(set_layer), METH_VARARGS, "set_layer(layer)\nSelect layer. 0=Raster, 1=Object"},
   {"swap_colors", FREE_FORWARDER(swap_colors), METH_NOARGS, "swap_colors()\nSwaps the foreground and background colors"},
   {"to_svg_path", to_svg_path, METH_VARARGS, "to_svg_path(path) -> string\nReturns an svg-path string describing a Path-object"},
   {"tool", FREE_FORWARDER(tool), METH_VARARGS, "tool([i])\nSelects the tool with the specified integer id, or returns the current id if none specified."},
   {"tool_raster_selection", FREE_FORWARDER(tool_raster_selection), METH_NOARGS, "tool_raster_selection()\nSelects the raster selection tool"},
   {"tool_object_selection", FREE_FORWARDER(tool_object_selection), METH_NOARGS, "tool_object_selection()\nSelects the object selection tool"},
   {"tool_pen", FREE_FORWARDER(tool_pen), METH_NOARGS, "tool_pen()\nSelects the pen tool"},
   {"tool_brush", FREE_FORWARDER(tool_brush), METH_NOARGS, "tool_brush()\nSelects the brush tool"}
    ,
    {"tool_path", FREE_FORWARDER(tool_path), METH_NOARGS, "tool_path()\nSelects the path tool"},
   {"tool_picker", FREE_FORWARDER(tool_picker), METH_NOARGS, "tool_picker()\nSelects the picker tool"},
   {"tool_line", FREE_FORWARDER(tool_line), METH_NOARGS, "tool_line()\nSelects the line tool"},
   {"tool_spline", FREE_FORWARDER(tool_spline), METH_NOARGS, "tool_spline()\nSelects the spline tool"},
   {"tool_rectangle", FREE_FORWARDER(tool_rectangle), METH_NOARGS, "tool_rectangle()\nSelects the rectangle tool"},
   {"tool_ellipse", FREE_FORWARDER(tool_ellipse), METH_NOARGS, "tool_ellipse()\nSelects the ellipse tool"},
   {"tool_polygon", FREE_FORWARDER(tool_polygon), METH_NOARGS, "tool_polygon()\nSelects the polygon tool"},
   {"tool_text", FREE_FORWARDER(tool_text), METH_NOARGS, "tool_text()\nSelects the text tool"},
   {"tool_fill", FREE_FORWARDER(tool_fill), METH_NOARGS, "tool_fill()\nSelects the fill tool"},
   {"update_settings", FREE_FORWARDER(update_settings), METH_VARARGS, "update_settings(settings)\n\nUpdates the active tool settings with the settings from the passed in Settings object."},
   {"grid", get_active_grid, METH_NOARGS, "A reference to the grid for the active image."},
#include "python/generate/output/generated-methods.hh"
   {0, 0, 0, 0}
 };

static struct PyModuleDef keyModule = {
  PyModuleDef_HEAD_INIT,
  "ifaint_key",
  "Key identifiers",
  -1, // m_size
  NULL, // m_methods
  NULL, // m_reload
  NULL, // m_traverse
  NULL, // m_clear
  NULL, // m_free
};

static PyObject* create_key_module(){
  PyObject* module = PyModule_Create(&keyModule);
  PyModule_AddIntConstant(module, "arrow_down", key::down);
  PyModule_AddIntConstant(module, "arrow_left", key::left);
  PyModule_AddIntConstant(module, "arrow_right", key::right);
  PyModule_AddIntConstant(module, "arrow_up", key::up);
  PyModule_AddIntConstant(module, "asterisk", key::asterisk);
  PyModule_AddIntConstant(module, "backspace", key::back);
  PyModule_AddIntConstant(module, "delete", key::del);
  PyModule_AddIntConstant(module, "end", key::end);
  PyModule_AddIntConstant(module, "f1", key::F1);
  PyModule_AddIntConstant(module, "f2", key::F2);
  PyModule_AddIntConstant(module, "f3", key::F3);
  PyModule_AddIntConstant(module, "f4", key::F4);
  PyModule_AddIntConstant(module, "f5", key::F5);
  PyModule_AddIntConstant(module, "f6", key::F6);
  PyModule_AddIntConstant(module, "f7", key::F7);
  PyModule_AddIntConstant(module, "f8", key::F8);
  PyModule_AddIntConstant(module, "f9", key::F9);
  PyModule_AddIntConstant(module, "f10", key::F10);
  PyModule_AddIntConstant(module, "f11", key::F11);
  PyModule_AddIntConstant(module, "f12", key::F12);
  PyModule_AddIntConstant(module, "home", key::home);
  PyModule_AddIntConstant(module, "num_minus", key::num_minus);
  PyModule_AddIntConstant(module, "num_plus", key::num_plus);
  PyModule_AddIntConstant(module, "paragraph", key::paragraph);
  PyModule_AddIntConstant(module, "pgdn", key::pgdn);
  PyModule_AddIntConstant(module, "pgup", key::pgup);
  PyModule_AddIntConstant(module, "space", key::space);
  return module;
}

static struct PyModuleDef modifierModule = {
  PyModuleDef_HEAD_INIT,
  "ifaint_mod",
  "Key modifier identifiers",
  -1, // m_size
  NULL, // m_methods
  NULL, // m_reload
  NULL, // m_traverse
  NULL, // m_clear
  NULL, // m_free
};

static PyObject* create_modifier_module(){
  PyObject* module = PyModule_Create(&modifierModule);
  PyModule_AddIntConstant(module, "alt", Alt.Raw());
  PyModule_AddIntConstant(module, "shift", Shift.Raw());
  PyModule_AddIntConstant(module, "ctrl", Ctrl.Raw());
  return module;
}


static struct PyModuleDef faintInterfaceModule = {
  PyModuleDef_HEAD_INIT,
  "ifaint",
  "ifaint\n\nThe built in functions and classes for faint-graphics-editor.\n",
  -1, // m_size
  faint_interface_methods, // m_methods
  NULL, // m_reload
  NULL, // m_traverse
  NULL, // m_clear
  NULL, // m_free
};

static PyObject* ifaintError;

PyMODINIT_FUNC PyInit_ifaint(){
  PyObject* module_ifaint = PyModule_Create(&faintInterfaceModule);
  assert(module_ifaint != nullptr);
  AddPyObjects(module_ifaint);
  ifaintError = PyErr_NewException("ifaint.error", NULL, NULL);
  Py_INCREF(ifaintError);
  PyModule_AddObject(module_ifaint, "error", ifaintError);
  PyModule_AddObject(module_ifaint, "key", create_key_module());
  PyModule_AddObject(module_ifaint, "mod", create_modifier_module());
  return module_ifaint;
}

} // namespace
