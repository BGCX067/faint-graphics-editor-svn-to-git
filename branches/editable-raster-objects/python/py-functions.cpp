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
#include "app/getappcontext.hh"
#include "gui/brightness-contrast-dialog.hh" // Fixme
#include "gui/resize-dialog.hh" // Fixme
#include "gui/rotate-dialog.hh" // Fixme
#include "gui/threshold-dialog.hh" // Fixme
#include "objects/objellipse.hh"
#include "objects/objpath.hh"
#include "objects/objraster.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "python/pythoninclude.hh"
#include "python/py-canvas.hh"
#include "python/py-format.hh"
#include "python/py-frame.hh"
#include "python/py-functions.hh"
#include "python/py-grid.hh"
#include "python/py-settings.hh"
#include "python/py-something.hh"
#include "python/py-util.hh"
#include "python/pyinterface.hh"
#include "python/py-app-objects.hh"
#include "python/generate/output/generated_methods.hh"
#include "util/canvasinterface.hh"
#include "util/keycode.hh"
#include "util/pathutil.hh"
#include "util/pattern.hh"
#include "util/settings.hh"
#include "util/util.hh"

PyObject* add_format( PyObject*, PyObject* args ){
  PyObject* loader;
  PyObject* saver;
  char* name;
  char* extension;
  if ( !PyArg_ParseTuple(args, "OOss", &loader, &saver, &name, &extension)) {
    return nullptr;
  }

  if ( loader == Py_None ){
    loader = nullptr;
  }
  else if ( !PyCallable_Check( loader ) ){
    PyErr_SetString(PyExc_TypeError, "Loader must be a callback or None:\nadd_format( loader, saver, name, extension)");
    return nullptr;
  }
  else {
    Py_INCREF(loader);
  }

  if ( saver == Py_None ){
    saver = nullptr;
  }
  else if ( !PyCallable_Check( saver ) ){
    PyErr_SetString(PyExc_TypeError, "Saver must be a callback or None");
    return nullptr;
  }
  else{
    Py_INCREF(saver);
  }

  if ( loader == nullptr && saver == nullptr ){
    PyErr_SetString(PyExc_ValueError, "saver and loader both None");
    return nullptr;
  }
  PyFileFormat* f = new PyFileFormat( load_callback_t(loader), save_callback_t(saver), label_t(name), extension_t(extension) );
  GetAppContext().AddFormat( f );
  return Py_BuildValue("");
}


PyObject* autosize_raster( PyObject*, PyObject* args ){
  PyObject* obj;
  if ( !PyArg_ParseTuple( args, "O", &obj ) ){
    PyErr_SetString(PyExc_TypeError, "autosize_text requires a text object" );
    return nullptr;
  }

  std::pair<CanvasInterface*, ObjRaster*> p = as_ObjRaster( obj );
  if ( p.second == nullptr ){
    PyErr_SetString(PyExc_ValueError, "autosize_raster requires a raster object" );
    return nullptr;
  }
  faint::python_run_command(p.first, crop_raster_object_command(p.second));
  return Py_BuildValue("");
}

PyObject* autosize_text( PyObject*, PyObject* args ){
  PyObject* obj;
  if ( !PyArg_ParseTuple( args, "O", &obj ) ){
    PyErr_SetString(PyExc_TypeError, "autosize_text requires a text object" );
    return nullptr;  }

  std::pair<CanvasInterface*, ObjText*> p = as_ObjText( obj );
  if ( p.second == nullptr ){
    PyErr_SetString(PyExc_ValueError, "autosize_text requires a text object" );
    return nullptr;
  }
  faint::python_run_command(p.first, crop_text_region_command(p.second));
  return Py_BuildValue("");
}

static PyObject* dialog_resize( PyObject*, PyObject* ){
  ResizeDialog dialog;
  GetAppContext().Show( dialog );
  return Py_BuildValue("");
}

static PyObject* dialog_rotate( PyObject*, PyObject* ){
  RotateDialog dialog;
  GetAppContext().Show( dialog );
  return Py_BuildValue("");
}

static PyObject* dialog_brightness_contrast( PyObject*, PyObject* ){
  BrightnessContrastDialog dialog;
  GetAppContext().Show( dialog );
  return Py_BuildValue("");
}

static PyObject* dialog_help( PyObject*, PyObject* ){
  GetAppContext().ShowHelpFrame();
  return Py_BuildValue("");
}

static PyObject* dialog_open_file( PyObject*, PyObject* ){
  GetAppContext().DialogOpenFile();
  return Py_BuildValue("");
}

static PyObject* dialog_threshold( PyObject*, PyObject* ){
  ThresholdDialog dialog;
  GetAppContext().Show( dialog );
  return Py_BuildValue("");
}

static PyObject* dialog_python_console( PyObject*, PyObject* ){
  GetAppContext().ShowPythonConsole();
  return Py_BuildValue("");
}

static PyObject* faint_quit( PyObject*, PyObject* ){
  GetAppContext().Quit();
  return Py_BuildValue("");
}

static PyObject* font_list(PyObject*, PyObject* ){
  std::vector<std::string> faceNames( available_font_facenames() );
  PyObject* list = PyList_New(faceNames.size());
  for ( size_t i = 0; i != faceNames.size(); i++ ){
    PyList_SetItem( list, i, PyString_FromString( faceNames[i].c_str() ) );
  }
  return list;
}

static PyObject* get_active_canvas( PyObject*, PyObject* ){
  return pythoned( GetAppContext().GetActiveCanvas() );
}

static PyObject* get_canvas_list( PyObject*, PyObject* ){
  AppContext& app = GetAppContext();
  size_t numCanvas = app.GetCanvasCount();
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i != numCanvas; i++ ){
    PyList_Append( py_list, pythoned(app.GetCanvas(i) ) );
  }
  return py_list;
}

static PyObject* get_bg( PyObject*, PyObject* ){
  return build_draw_source( GetAppContext().Get( ts_BgCol ) );
}

static PyObject* get_fg( PyObject*, PyObject* ){
  return build_draw_source( GetAppContext().Get( ts_FgCol ) );
}

static PyObject* get_font(PyObject*, PyObject* ){
  StrSetting::ValueType font = GetAppContext().Get( ts_FontFace );
  const char* str = font.c_str();
  return Py_BuildValue( "s", str );
}

static PyObject* get_ini_dir( PyObject*, PyObject* ){
  return Py_BuildValue("s", get_home_dir().c_str());
}

static PyObject* get_ini_path( PyObject*, PyObject* ){
  return Py_BuildValue("s", get_ini_file_path().c_str());
}

static PyObject* get_layer( PyObject*, PyObject* ){
  return Py_BuildValue("i", static_cast<int>( GetAppContext().GetLayerType()) );
}

static PyObject* get_mouse_pos_screen( PyObject*, PyObject* ){
  Point mousePos( GetAppContext().GetMousePos() );
  return Py_BuildValue("(ii)", (int)mousePos.x, (int)mousePos.y );
}

static PyObject* get_settings( PyObject*, PyObject* ){
  return pythoned( GetAppContext().GetToolSettings() );
}

static PyObject* set_active_canvas( PyObject*, PyObject* args ){
  PyObject* o = nullptr;
  if ( !PyArg_ParseTuple( args, "O", &o ) ){
    return nullptr;
  }
  if ( !PyObject_IsInstance(o, (PyObject*)&CanvasType) ){
    PyErr_SetString(PyExc_TypeError, "Argument must be a Canvas");
    return nullptr;
  }

  canvasObject* canvas = (canvasObject*)o;
  GetAppContext().SetActiveCanvas(canvas->id);
  return Py_BuildValue("");
}

static PyObject* set_bg( PyObject*, PyObject* args ){
  faint::DrawSource src;
  if ( !parse_draw_source( args, src ) ){
    return nullptr;
  }

  GetAppContext().Set( ts_BgCol, src );
  return Py_BuildValue("");
}

static PyObject* set_fg( PyObject*, PyObject* args){
  faint::DrawSource src;
  if ( !parse_draw_source( args, src ) ){
    return nullptr;
  }

  GetAppContext().Set( ts_FgCol, src );
  return Py_BuildValue("");
}

static PyObject* set_layer( PyObject*, PyObject* args){
  int layer = 0;
  if ( !PyArg_ParseTuple( args, "i", &layer ) ){
    return nullptr;
  }

  if ( valid_layerstyle(layer) ){
    GetAppContext().SetLayer( (to_layerstyle(layer)) );
    return Py_BuildValue("");
  }

  std::stringstream ss;
  ss << "Invalid value for layer.\nAcceptable values are:\n" << to_int(Layer::RASTER) // Fixme
     << " - Raster layer\n " << to_int(Layer::OBJECT) // Fixme
     << " - Object layer\n";
  PyErr_SetString( PyExc_ValueError, ss.str().c_str() );
  return nullptr;
}

static PyObject* incomplete_command(PyObject*, PyObject* ){
  GetAppContext().PythonContinuation();
  return Py_BuildValue("");
}

static PyObject* int_bind_key( PyObject*, PyObject* args ){
  // Inform the C++ side that the key is bound
  int key = 0;
  int modifiers = 0;
  if ( !PyArg_ParseTuple( args, "ii", &key, &modifiers ) ){
    PyErr_SetString(PyExc_TypeError, "Faint Internal error: Invalid key passed to int_bind_key");
    return nullptr;
  }
  GetAppContext().Bind( key, modifiers );
  return Py_BuildValue("");
}

static PyObject* int_bind_key_global( PyObject*, PyObject* args ){
  // Inform the C++ side that the key is bound globally
  int key = 0;
  int modifiers = 0;
  if ( !PyArg_ParseTuple( args, "ii", &key, &modifiers ) ){
    PyErr_SetString(PyExc_TypeError, "Faint Internal error: Invalid key passed to int_bind_key_global");
    return nullptr;
  }
  GetAppContext().Bind( key, modifiers, true );
  return Py_BuildValue("");
}

static PyObject* int_faint_print(PyObject*, PyObject* args) {
  char* strn = nullptr;
  if ( !PyArg_ParseTuple(args, "s", &strn)){
    return nullptr;
  }
  GetAppContext().PythonIntFaintPrint( std::string(strn) );
  return Py_BuildValue("");
}

static PyObject* int_get_key( PyObject*, PyObject* ){
  GetAppContext().PythonGetKey();
  return Py_BuildValue("");
}

static PyObject* int_get_key_name( PyObject*, PyObject* args ){
  int key = 0;
  if ( !PyArg_ParseTuple(args, "i", &key) ){
    return nullptr;
  }
  std::string name( key::name(key) );
  return Py_BuildValue("s", name.c_str());
}

static PyObject* int_unbind_key( PyObject*, PyObject* args ){
  // Inform the C++ side that the key is unbound
  int key = 0;
  int modifiers = 0;
  if ( !PyArg_ParseTuple( args, "ii", &key, &modifiers ) ){
    PyErr_SetString(PyExc_TypeError, "Faint Internal error: Invalid key passed to int_unbind_key");
    return nullptr;
  }
  GetAppContext().Unbind( key, modifiers );
  return Py_BuildValue("");
}

PyObject* list_formats( PyObject*, PyObject* ){
  std::vector<Format*> formats = GetAppContext().GetFormats();
  PyObject* list = PyList_New( formats.size() );
  for ( size_t i = 0; i != formats.size(); i++ ){
    const Format* frm = formats[i];
    PyObject* o = Py_BuildValue("ss", frm->GetLabel().c_str(), frm->GetDefaultExtension().c_str() );
    PyList_SetItem( list, i, o );
  }
  return list;
}

static PyObject* print_to_stdout( PyObject*, PyObject* args ){
  char* s;
  if ( !PyArg_ParseTuple( args,"s",&s ) ){
    return nullptr;
  }
  std::cout << s << std::endl;
  return Py_BuildValue("");
}

static PyObject* read_eval_print_done(PyObject*, PyObject* ){
  AppContext& app( GetAppContext() );
  app.PythonDone();
  app.PythonNewPrompt();
  GetAppContext().PythonDone();
  return Py_BuildValue("");
}

static PyObject* set_font(PyObject*, PyObject* args){
  char* strPtr = nullptr;
  if ( !PyArg_ParseTuple( args, "s", &strPtr ) ){
    return nullptr;
  }
  if ( !valid_facename(strPtr) ){
    PyErr_SetString( PyExc_ValueError, "Invalid facename" );
    return nullptr;
  }
  AppContext& ctx = GetAppContext();
  ctx.Set( ts_FontFace, std::string( strPtr ) );
  return Py_BuildValue("");
}

static PyObject* swap_colors( PyObject*, PyObject* ){
  AppContext& app(GetAppContext());
  faint::DrawSource fg(app.Get( ts_FgCol ));
  faint::DrawSource bg(app.Get( ts_BgCol ));
  app.Set( ts_FgCol, bg );
  app.Set( ts_BgCol, fg );
  return Py_BuildValue("");
}

static PyObject* tool( PyObject*, PyObject* args ){
  if ( PySequence_Length(args) == 0 ){
    // Return the current id when called with no parameters
    return Py_BuildValue("i", to_int(GetAppContext().GetToolId()));
  }

  int id = 0;
  if ( !PyArg_ParseTuple( args, "i", &id ) ){
    return nullptr;
  }
  if ( id < to_int(ToolId::MIN_ID) || to_int(ToolId::MAX_VALID_ID) < id ){
    char error[255];
    sprintf( error, "Invalid tool identifier. Valid tool identifiers are %d to %d.", to_int(ToolId::MIN_ID), to_int(ToolId(ToolId::MAX_VALID_ID)));
    PyErr_SetString( PyExc_ValueError, error );
    return nullptr;
  }

  AppContext& ctx = GetAppContext();
  ctx.SelectTool( to_tool_id( id ) );
  return Py_BuildValue("");
}

static PyObject* tool_raster_selection( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::RECTANGLE_SELECTION);
  return Py_BuildValue("");
}

static PyObject* tool_object_selection( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::OBJECT_SELECTION);
  return Py_BuildValue("");
}

static PyObject* tool_pen( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::PEN);
  return Py_BuildValue("");
}

static PyObject* tool_brush( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::BRUSH);
  return Py_BuildValue("");
}

static PyObject* tool_picker( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::PICKER);
  return Py_BuildValue("");
}

static PyObject* tool_line( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::LINE);
  return Py_BuildValue("");
}

static PyObject* tool_spline( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::SPLINE);
  return Py_BuildValue("");
}

static PyObject* tool_rectangle( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::RECTANGLE);
  return Py_BuildValue("");
}

static PyObject* tool_ellipse( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::ELLIPSE);
  return Py_BuildValue("");
}

static PyObject* tool_polygon( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::POLYGON);
  return Py_BuildValue("");
}

static PyObject* tool_text( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::TEXT);
  return Py_BuildValue("");
}

static PyObject* tool_fill( PyObject*, PyObject* ){
  GetAppContext().SelectTool(ToolId::FLOOD_FILL);
  return Py_BuildValue("");
}

static PyObject* to_svg_path( PyObject*, PyObject* args ){
  PyObject* obj = nullptr;
  if ( ! PyArg_ParseTuple( args, "O", &obj ) ){
    return nullptr;
  }
  ObjPath* path = as_ObjPath( obj );
  if ( path != nullptr ){
    std::string pathStr( points_to_svg_path_string( path->GetPathPoints() ) );
    return Py_BuildValue("s", pathStr.c_str());
  }
  PyErr_Clear();
  ObjSpline* spline = as_ObjSpline( obj );
  if ( spline != nullptr ){
    std::string pathStr( points_to_svg_path_string( spline_to_svg_path( spline->GetSplinePoints() ).GetPoints() ) );
    return Py_BuildValue("s", pathStr.c_str());
  }
  ObjEllipse* ellipse = as_ObjEllipse( obj );
  if ( ellipse != nullptr ){
    std::stringstream ss;
    std::vector<Point> v = ellipse_as_path( ellipse->GetTri() );
    ss << "M " << v[0].x << "," << v[0].y << " ";
    for ( size_t i = 1; i != v.size(); i+= 3 ){
      ss << "C " <<
        v[i].x << "," << v[i].y << " " <<
        v[i + 1].x << "," << v[i + 1].y << " " <<
        v[i + 2].x << "," << v[i + 2].y << " ";
    }
    ss << "z";
    return Py_BuildValue("s", ss.str().c_str() );
  }
  PyErr_SetString(PyExc_ValueError, "Object can not be transformed to path" );
  return nullptr;
}

static PyObject* update_settings(PyObject*, PyObject* args ){
  PyObject* o = nullptr;
  if ( PyArg_ParseTuple( args, "O", &o ) ){
    if ( PyObject_IsInstance( o, (PyObject*)&SettingsType ) ){
      settingsObject* s = (settingsObject*)o;
      GetAppContext().UpdateToolSettings( *(s->settings) );
      return Py_BuildValue("");
    }
    else {
      PyErr_SetString( PyExc_TypeError, "Invalid object" );
      return nullptr;
    }
  }
  else {
    PyErr_SetString( PyExc_TypeError, "Invalid parameters" );
    return nullptr;
  }
}

static PyObject* one_color_bg( PyObject*, PyObject* args ){
  PyObject* obj;
  if ( !PyArg_ParseTuple( args, "O", &obj ) ){
    return nullptr;
  }
  if ( !PyObject_IsInstance( obj, (PyObject*)&FrameType) ){
    PyErr_SetString(PyExc_TypeError, "Object is not a frame");
    return nullptr;
  }

  frameObject* frame = (frameObject*)obj;
  if ( !frame_ok(frame) ){
    return nullptr;
  }
  const faint::Image& image = frame->canvas->GetFrameById(frame->frameId);
  return python_bool( is_blank(image.GetBitmap()) );
}

static PyObject* bitmap_from_png_string( PyObject*, PyObject* args ){
  faint::Bitmap bmp;
  if ( !parse_png_bitmap(args, bmp ) ){
    return nullptr;
  }
  return build_bitmap(bmp);
}

static PyObject* get_active_grid( PyObject*, PyObject* ){
  return py_grid_canvas(nullptr);
}

static PyObject* get_pattern_status( PyObject*, PyObject* ){
  std::map<int,int> status = faint::pattern_status();
  std::stringstream ss;
  for ( const auto& idToCount : status ){
    ss << idToCount.first << "=" << idToCount.second << ",";
  }
  return Py_BuildValue("s", ss.str().c_str());
}

static PyMethodDef faint_interface_methods[] = {
   {"add_format", add_format, METH_VARARGS, "add_format(load_callback, save_callback, name, extension)\nAdd a file format for loading and/or saving.\n\nEither load_callback or save_callback can be None.\nname is the name of the file format, extension is the file extension, used to identify the format.\n\nThe load_callback receives a filename and an ImageProps object:\n  my_load_callback(filename, imageProps)\n\nThe save_callback receives a target filename and a Canvas:\n  my_save_callback(filename, canvas)" },
   {"autosize_raster", autosize_raster, METH_VARARGS, "autosize_raster(raster_object)\n"},
   {"autosize_text", autosize_text, METH_VARARGS, "autosize_text(text_object)\n"},
   {"bitmap_from_png_string", bitmap_from_png_string, METH_VARARGS, "bitmap_from_png_string(s) -> bmp\nCreates a bitmap from the PNG string"},
   {"cout", print_to_stdout, METH_VARARGS, "Uses std::cout"},
   {"dialog_brightness_contrast", dialog_brightness_contrast, METH_NOARGS, "Show the brightness contrast dialog"},
   {"dialog_help", dialog_help, METH_NOARGS, "Show the Help dialog"},
   {"dialog_open_file", dialog_open_file, METH_NOARGS, "Show the Open file dialog"},
   {"dialog_python_console", dialog_python_console, METH_NOARGS, "Show the Python console"},
   {"dialog_resize", dialog_resize, METH_NOARGS, "Show the resize dialog"},
   {"dialog_rotate", dialog_rotate, METH_NOARGS, "Show the rotate dialog"},
   {"dialog_threshold", dialog_threshold, METH_NOARGS, "Show the threshold dialog."},
   {"faint_quit", faint_quit, METH_NOARGS, "Exit faint"},
   {"get_active_image", get_active_canvas, METH_VARARGS, "get_active_image() -> Canvas \nReturns the active (currently edited) image."},
   {"set_active_image", set_active_canvas, METH_VARARGS, "set_active_image(canvas) -> \nActivates (selects in a tab) the specified image."},
   {"get_ini_path", get_ini_path, METH_VARARGS, "get_ini_path() -> s\nReturns the path to the users ini file"},
   {"get_ini_dir", get_ini_dir, METH_VARARGS, "get_ini_path() -> s\nReturns the path to the directory with the users ini file"},
   {"get_bg", get_bg, METH_VARARGS, "get_bg()->bg\nGet the Background color, Gradient or Pattern."},
   {"get_fg", get_fg, METH_VARARGS, "get_fg()->fg\nReturns the foreground color, Gradient or Pattern."},
   {"get_font", get_font, METH_NOARGS, "get_font()\n\nReturns the active font face name."},
   {"get_layer", get_layer, METH_NOARGS, "Returns the layer index"},
   {"get_mouse_pos_screen", get_mouse_pos_screen, METH_NOARGS, "Returns the mouse pointer position"},
   {"get_settings", get_settings, METH_NOARGS, "get_settings() -> Settings \nReturns the current tool settings"},
   {"int_bind_key", int_bind_key, METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_bind_key_global", int_bind_key_global, METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_faint_print", int_faint_print, METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_get_key", int_get_key, METH_VARARGS, "Faint framework function, not indended for use."},
   {"int_get_key_name", int_get_key_name, METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_incomplete_command", incomplete_command, METH_VARARGS, "Faint framework function, not intended for use"},
   {"int_ran_command", read_eval_print_done, METH_VARARGS, "Faint framework function, not intended for use."},
   {"int_unbind_key", int_unbind_key, METH_VARARGS, "Faint framework function, not intended for use."},
   {"list_fonts", font_list, METH_NOARGS, "list_fonts() -> list of fonts\n\nReturns a list with the names of all available fonts."},
   {"list_formats", list_formats, METH_NOARGS, "List available file formats."},
   {"list_images", get_canvas_list, METH_VARARGS, "list_images() -> [c1, c2, ...]\nRetrieve a list of all opened images as Canvas-objects."},
   {"one_color_bg", one_color_bg, METH_VARARGS, "one_color_bg(frame) -> b\nTrue if the frame background consists of a single color\n"},
   {"pattern_status", get_pattern_status, METH_VARARGS, "Fixme"},
   {"set_bg", set_bg, METH_VARARGS,
    "set_bg(c)\n\nSet the background color. Accepts a color tuple, a Gradient or a Pattern."},
   {"set_fg", set_fg, METH_VARARGS,
    "set_fg(r,g,b,[a]) Set the foreground color. Accepts a color tuple, a Gradient or a Pattern."},
   {"set_font", set_font, METH_VARARGS, "set_font(fontname)\n\nSet the active font. Takes one string parameter, the font face name."},
   {"set_layer", set_layer, METH_VARARGS, "set_layer(layer)\nSelect layer. 0=Raster, 1=Object"},
   {"swap_colors", swap_colors, METH_NOARGS, "swap_colors()\nSwaps the foreground and background colors"},
   {"to_svg_path", to_svg_path, METH_VARARGS, "to_svg_path(path) -> string\nReturns an svg-path string describing a Path-object"},
   {"tool", tool, METH_VARARGS, "tool([i])\nSelects the tool with the specified integer id, or returns the current id if none specified."},
   {"tool_raster_selection", tool_raster_selection, METH_NOARGS, "tool_raster_selection()\nSelects the raster selection tool"},
   {"tool_object_selection", tool_object_selection, METH_NOARGS, "tool_object_selection()\nSelects the object selection tool"},
   {"tool_pen", tool_pen, METH_NOARGS, "tool_pen()\nSelects the pen tool"},
   {"tool_brush", tool_brush, METH_NOARGS, "tool_brush()\nSelects the brush tool"},
   {"tool_picker", tool_picker, METH_NOARGS, "tool_picker()\nSelects the picker tool"},
   {"tool_line", tool_line, METH_NOARGS, "tool_line()\nSelects the line tool"},
   {"tool_spline", tool_spline, METH_NOARGS, "tool_spline()\nSelects the spline tool"},
   {"tool_rectangle", tool_rectangle, METH_NOARGS, "tool_rectangle()\nSelects the rectangle tool"},
   {"tool_ellipse", tool_ellipse, METH_NOARGS, "tool_ellipse()\nSelects the ellipse tool"},
   {"tool_polygon", tool_polygon, METH_NOARGS, "tool_polygon()\nSelects the polygon tool"},
   {"tool_text", tool_text, METH_NOARGS, "tool_text()\nSelects the text tool"},
   {"tool_fill", tool_fill, METH_NOARGS, "tool_fill()\nSelects the fill tool"},
   {"update_settings", update_settings, METH_VARARGS, "update_settings(settings)\n\nUpdates the active tool settings with the settings from the passed in Settings object."},
   {"active_grid", get_active_grid, METH_NOARGS, "Get default grid"}, // Fixme: Better docstr
   {0, 0, 0, 0}
 };

void init_faint_module(){
  PyObject* module_ifaint = Py_InitModule("ifaint", faint_interface_methods);
  Py_InitModule("ifaint", generated_methods);
  AddPyObjects( module_ifaint );
}
