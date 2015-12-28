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
#include "objects/objellipse.hh"
#include "objects/objpath.hh"
#include "objects/objraster.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "python/pythoninclude.hh"
#include "python/py_canvas.hh"
#include "python/py_functions.hh"
#include "python/py_grid.hh"
#include "python/py_settings.hh"
#include "python/py_something.hh"
#include "python/py_util.hh"
#include "python/pyinterface.hh"
#include "python/python_format.hh"
#include "python/py_app_objects.hh"
#include "python/generate/output/generated_methods.hh"
#include "util/canvasinterface.hh"
#include "util/keycode.hh"
#include "util/pathutil.hh"
#include "util/settings.hh"
#include "util/util.hh"

PyObject* add_format( PyObject*, PyObject* args ){
  PyObject* loader;
  PyObject* saver;
  char* name;
  char* extension;
  if ( !PyArg_ParseTuple(args, "OOss", &loader, &saver, &name, &extension)) {
    return 0;
  }

  if ( loader == Py_None ){
    loader = 0;
  }
  else if ( !PyCallable_Check( loader ) ){
    PyErr_SetString(PyExc_TypeError, "Loader must be a callback or None:\nadd_format( loader, saver, name, extension)");
    return 0;
  }
  else {
    Py_INCREF(loader);
  }

  if ( saver == Py_None ){
    saver = 0;
  }
  else if ( !PyCallable_Check( saver ) ){
    PyErr_SetString(PyExc_TypeError, "Saver must be a callback or None");
    return 0;
  }
  else{
    Py_INCREF(saver);
  }

  if ( loader == 0 && saver == 0 ){
    PyErr_SetString(PyExc_ValueError, "saver and loader both None");
    return 0;
  }
  PyFileFormat* f = new PyFileFormat( load_callback_t(loader), save_callback_t(saver), label_t(name), extension_t(extension) );
  GetAppContext().AddFormat( f );
  return Py_BuildValue("");
}


PyObject* autosize_raster( PyObject*, PyObject* args ){
  PyObject* obj;
  if ( !PyArg_ParseTuple( args, "O", &obj ) ){
    PyErr_SetString(PyExc_TypeError, "autosize_text requires a text object" );
    return 0;
  }

  std::pair<CanvasInterface*, ObjRaster*> p = as_ObjRaster( obj );
  if ( p.second == 0 ){
    PyErr_SetString(PyExc_ValueError, "autosize_raster requires a raster object" );
    return 0;
  }
  faint::python_run_command(p.first, crop_raster_object_command(p.second));
  return Py_BuildValue("");
}

PyObject* autosize_text( PyObject*, PyObject* args ){
  PyObject* obj;
  if ( !PyArg_ParseTuple( args, "O", &obj ) ){
    PyErr_SetString(PyExc_TypeError, "autosize_text requires a text object" );
    return 0;  }

  std::pair<CanvasInterface*, ObjText*> p = as_ObjText( obj );
  if ( p.second == 0 ){
    PyErr_SetString(PyExc_ValueError, "autosize_text requires a text object" );
    return 0;
  }
  faint::python_run_command(p.first, crop_text_region_command(p.second));
  return Py_BuildValue("");
}

static PyObject* dialog_help( PyObject*, PyObject* ){
  GetAppContext().ShowHelpFrame();
  return Py_BuildValue("");
}

static PyObject* dialog_python_console( PyObject*, PyObject* ){
  GetAppContext().ShowPythonConsole();
  return Py_BuildValue("");
}

static PyObject* dialog_open_file( PyObject*, PyObject* ){
  GetAppContext().DialogOpenFile();
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

PyObject* get_active_canvas( PyObject*, PyObject* ){
  return pythoned( GetAppContext().GetActiveCanvas() );
}

PyObject* get_canvas_list( PyObject*, PyObject* ){
  AppContext& app = GetAppContext();
  size_t numCanvas = app.GetCanvasCount();
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i != numCanvas; i++ ){
    PyList_Append( py_list, pythoned(app.GetCanvas(i) ) );
  }
  return py_list;
}

PyObject* get_bgcol( PyObject*, PyObject* ){
  return build_color_tuple( GetAppContext().Get( ts_BgCol ) );
}

PyObject* get_fgcol( PyObject*, PyObject* ){
  return build_color_tuple( GetAppContext().Get( ts_FgCol ) );
}

PyObject* get_font(PyObject*, PyObject* ){
  StrSetting::ValueType font = GetAppContext().Get( ts_FontFace );
  const char* str = font.c_str();
  return Py_BuildValue( "s", str );
}

PyObject* get_ini_dir( PyObject*, PyObject* ){
  return Py_BuildValue("s", get_home_dir().c_str());
}

PyObject* get_ini_path( PyObject*, PyObject* ){
  return Py_BuildValue("s", get_ini_file_path().c_str());
}

PyObject* get_mouse_pos_screen( PyObject*, PyObject* ){
  Point mousePos( GetAppContext().GetMousePos() );
  return Py_BuildValue("(ii)", (int)mousePos.x, (int)mousePos.y );
}

PyObject* get_settings( PyObject*, PyObject* ){
  return pythoned( GetAppContext().GetToolSettings() );
}

PyObject* set_active_canvas( PyObject*, PyObject* args ){
  PyObject* o = 0;
  if ( !PyArg_ParseTuple( args, "O", &o ) ){
    return 0;
  }
  if ( !PyObject_IsInstance(o, (PyObject*)&CanvasType) ){
    PyErr_SetString(PyExc_TypeError, "Argument must be a Canvas");
    return 0;
  }
  canvasObject* canvas = (canvasObject*)o;
  GetAppContext().SetActiveCanvas(canvas->id);
  return Py_BuildValue("");
}

PyObject* set_bgcol( PyObject*, PyObject* args ){
  faint::Color color;
  if ( !parse_color( args, color ) ){
    return 0;
  }
  GetAppContext().Set( ts_BgCol, color );
  return Py_BuildValue("");
}

PyObject* set_fgcol( PyObject*, PyObject* args){
  faint::Color color;
  if ( !parse_color( args, color ) ){
    return 0;
  }
  GetAppContext().Set( ts_FgCol, color );
  return Py_BuildValue("");
}

PyObject* set_layer( PyObject*, PyObject* args){
  int layer = 0;
  if ( !PyArg_ParseTuple( args, "i", &layer ) ){
    return 0;
  }
  if ( valid_layer(layer) ){
    GetAppContext().SetLayer( (to_layer(layer)) );
    return Py_BuildValue("");
  }

  std::stringstream ss;
  ss << "Invalid value for layer.\nAcceptable values are:\n" << Layer::RASTER << " - Raster layer\n " << Layer::OBJECT << " - Object layer\n";
  PyErr_SetString( PyExc_ValueError, ss.str().c_str() );
  return 0;
}

static PyObject* incomplete_command(PyObject*, PyObject* ){
  GetAppContext().PythonContinuation();
  return Py_BuildValue("");
}

static PyObject* int_bind_key( PyObject*, PyObject* args ){
  // Inform the C++ side that the key is bound
  int key=0;
  int modifiers=0;
  if ( !PyArg_ParseTuple( args, "ii", &key, &modifiers ) ){
    PyErr_SetString(PyExc_TypeError, "Faint Internal error: Invalid key passed to int_bind_key");
    return 0;
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
    return 0;
  }
  GetAppContext().Bind( key, modifiers, true );
  return Py_BuildValue("");
}

static PyObject* int_faint_print(PyObject*, PyObject* args) {
  char* strn = 0;
  if ( !PyArg_ParseTuple(args, "s", &strn)){
    return 0;
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
    return 0;
  }
  std::string name( key::name(key) );
  return Py_BuildValue("s", name.c_str());
}

static PyObject* int_unbind_key( PyObject*, PyObject* args ){
  // Inform the C++ side that the key is unbound
  int key=0;
  int modifiers=0;
  if ( !PyArg_ParseTuple( args, "ii", &key, &modifiers ) ){
    PyErr_SetString(PyExc_TypeError, "Faint Internal error: Invalid key passed to int_unbind_key");
    return 0;
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
    return 0;
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

PyObject* set_font(PyObject*, PyObject* args){
  char* strPtr = 0;
  if ( !PyArg_ParseTuple( args, "s", &strPtr ) ){
    return 0;
  }
  if ( !valid_facename(strPtr) ){
    PyErr_SetString( PyExc_ValueError, "Invalid facename" );
    return 0;
  }
  AppContext& ctx = GetAppContext();
  ctx.Set( ts_FontFace, std::string( strPtr ) );
  return Py_BuildValue("");
}

PyObject* tool( PyObject*, PyObject* args ){
  int toolId = 0;
  if ( PySequence_Length(args) == 0 ){
    return Py_BuildValue("i", static_cast<int>(GetAppContext().GetToolId() ) );
  }
  if ( !PyArg_ParseTuple( args, "i", &toolId ) ){
    return 0;
  }
  if ( !( T_MIN_ID <= toolId && toolId <= T_MAX_VALID_ID ) ){
    char error[255];
    sprintf( error, "Invalid tool identifier. Valid tool identifiers are %d to %d.", T_MIN_ID, T_MAX_VALID_ID );
    PyErr_SetString( PyExc_ValueError, error );
    return 0;
  }
  else {
    AppContext& ctx = GetAppContext();
    ctx.SelectTool(  static_cast<ToolId>( toolId ) );
  }
  return Py_BuildValue("");
}

PyObject* to_svg_path( PyObject*, PyObject* args ){
  PyObject* obj = 0;
  if ( ! PyArg_ParseTuple( args, "O", &obj ) ){
    return 0;
  }
  ObjPath* path = as_ObjPath( obj );
  if ( path != 0 ){
    std::string pathStr( points_to_svg_path_string( path->GetPathPoints() ) );
    return Py_BuildValue("s", pathStr.c_str());
  }
  PyErr_Clear();
  ObjSpline* spline = as_ObjSpline( obj );
  if ( spline != 0 ){
    std::string pathStr( points_to_svg_path_string( spline_to_svg_path( spline->GetSplinePoints() ).GetPoints() ) );
    return Py_BuildValue("s", pathStr.c_str());
  }
  ObjEllipse* ellipse = as_ObjEllipse( obj );
  if ( ellipse != 0 ){
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
  return 0;
}

PyObject* update_settings(PyObject*, PyObject* args ){
  PyObject* o = 0;
  if ( PyArg_ParseTuple( args, "O", &o ) ){
    if ( PyObject_IsInstance( o, (PyObject*)&SettingsType ) ){
      settingsObject* s = (settingsObject*)o;
      GetAppContext().UpdateToolSettings( *(s->settings) );
      return Py_BuildValue("");
    }
    else {
      PyErr_SetString( PyExc_TypeError, "Invalid object" );
      return 0;
    }
  }
  else {
    PyErr_SetString( PyExc_TypeError, "Invalid parameters" );
    return 0;
  }
}

PyObject* one_color_bg( PyObject*, PyObject* args ){
  PyObject* pyCanvas;
  if ( !PyArg_ParseTuple( args, "O", &pyCanvas ) ){
    return 0;
  }
  CanvasInterface* canvas = as_Canvas( pyCanvas );
  if ( canvas == 0 ){
    return 0;
  }

  const faint::Bitmap& bmp = canvas->GetBitmap();
  return python_bool( is_blank(bmp) );
}

PyObject* get_active_grid( PyObject*, PyObject* ){
  return py_grid_canvas(0);
}

static PyMethodDef faint_interface_methods[] = {
   {"add_format", add_format, METH_VARARGS, "add_format(load_callback, save_callback, name, extension)\nAdd a file format for loading and/or saving.\n\nEither load_callback or save_callback can be None.\nname is the name of the file format, extension is the file extension, used to identify the format.\n\nThe load_callback receives a filename and an ImageProps object:\n  my_load_callback(filename, imageProps)\n\nThe save_callback receives a target filename and a a CanvasInterface:\n  my_save_callback(filename, canvas)" },
   {"autosize_raster", autosize_raster, METH_VARARGS, "autosize_raster(raster_object)\n"},
   {"autosize_text", autosize_text, METH_VARARGS, "autosize_text(text_object)\n"},
   {"cout", print_to_stdout, METH_VARARGS, "Uses std::cout"},
   {"dialog_help", dialog_help, METH_NOARGS, "Show the Help dialog"},
   {"dialog_open_file", dialog_open_file, METH_NOARGS, "Show the Open file dialog"},
   {"dialog_python_console", dialog_python_console, METH_NOARGS, "Show the Python console"},
   {"faint_quit", faint_quit, METH_NOARGS, "Exit faint"},
   {"get_active_image", get_active_canvas, METH_VARARGS, "get_active_image() -> Canvas \nReturns the active (currently edited) image."},
   {"set_active_image", set_active_canvas, METH_VARARGS, "set_active_image(canvas) -> \nActivates (selects in a tab) the specified image."},
   {"get_ini_path", get_ini_path, METH_VARARGS, "get_ini_path() -> s\nReturns the path to the users ini file"},
   {"get_ini_dir", get_ini_dir, METH_VARARGS, "get_ini_path() -> s\nReturns the path to the directory with the users ini file"},
   {"get_bgcol", get_bgcol, METH_VARARGS, "Get the Background color."},
   {"get_fgcol", get_fgcol, METH_VARARGS, "Returns the foreground color."},
   {"get_font", get_font, METH_NOARGS, "get_font()\n\nReturns the active font face name."},
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
   {"one_color_bg", one_color_bg, METH_VARARGS, "true if the image background consists of a single color\n"},
   {"set_bgcol", set_bgcol, METH_VARARGS, "Set the background color. Takes a color tuple."},
   {"set_fgcol", set_fgcol, METH_VARARGS, "set_fgcol(r,g,b,[a]) Set the foreground color."},
   {"set_font", set_font, METH_VARARGS, "set_font(fontname)\n\nSet the active font. Takes one string parameter, the font face name."},
   {"set_layer", set_layer, METH_VARARGS, "set_layer(layer)\nSelect layer. 0=Raster, 1=Object"},
   {"to_svg_path", to_svg_path, METH_VARARGS, "to_svg_path(path) -> string\nReturns an svg-path string describing a Path-object"},
   {"tool", tool, METH_VARARGS, "tool(i)\n\nSelect tool by index."},
   {"update_settings", update_settings, METH_VARARGS, "update_settings(settings)\n\nUpdates the active tool settings with the settings from the passed in Settings object."},
   {"active_grid", get_active_grid, METH_NOARGS, "Get default grid"}, // Fixme: Better docstr
   {NULL, NULL, 0, NULL}
 };

void init_faint_module(){
  PyObject* module_ifaint = Py_InitModule("ifaint", faint_interface_methods);
  Py_InitModule("ifaint", generated_methods);
  AddPyObjects( module_ifaint );
}
