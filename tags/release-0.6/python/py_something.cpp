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

#include <vector>
#include <string>
#include "app/getappcontext.hh"
#include "commands/cmdchangesetting.hh"
#include "commands/cmdupdatesettings.hh"
#include "commands/textentrycommand.hh"
#include "commands/tricommand.hh"
#include "objects/objcomposite.hh"
#include "objects/objline.hh"
#include "objects/objpolygon.hh"
#include "objects/objraster.hh"
#include "objects/objrectangle.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "python/pythoninclude.hh"
#include "python/py_canvas.hh"
#include "python/py_settings.hh"
#include "python/py_something.hh"
#include "python/py_tri.hh"
#include "python/py_util.hh"
#include "python/pyinterface.hh"
#include "util/canvasinterface.hh"
#include "util/util.hh"

PyObject* pythoned( Object* faintObj, CanvasInterface* canvas ){
  smthObject* smth = (smthObject*)SmthType.tp_alloc( &SmthType, 0 );
  smth->obj = faintObj;
  smth->objectId = faintObj->GetId();
  smth->canvas = canvas;
  smth->canvasId = canvas->GetId();
  return (PyObject*)smth;
}

static PyObject* Smth_update_settings( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  settingsObject* settings;
  if ( !PyArg_ParseTuple( args, "O!", &SettingsType, &settings ) ){
    return 0;
  }

  NewSettings newSettings(New(*(settings->settings)));
  OldSettings oldSettings(self->obj->GetSettings());
  faint::python_run_command(self->canvas, new CmdUpdateSettings(self->obj, newSettings, oldSettings));
  return Py_BuildValue("");
}

static PyObject* Smth_moveto(smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  Point pt;
  if ( !parse_point( args, &pt ) ){
    return 0;
  }

  Object* obj = self->obj;
  Tri oldTri( obj->GetTri() );
  Point diff = pt - oldTri.P0();
  Tri newTri( translated( oldTri, diff.x, diff.y ) );

  faint::python_run_command( self->canvas, new TriCommand( obj,
      New(newTri), Old(oldTri) ) );
  return Py_BuildValue("");
}

static PyObject* Smth_rotate( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  faint::radian rad;
  if ( !parse_radian( args, &rad ) ){
    return 0;
  }

  Object* obj = self->obj;
  Tri oldTri( obj->GetTri() );
  Tri newTri( rotated( oldTri, rad, oldTri.P3() ) ); // Fixme: P3 is a rather arbitrary origin

  faint::python_run_command( self->canvas,
    new TriCommand( obj, New(newTri), Old(oldTri) ) );
  return Py_BuildValue("");
}

static PyObject* Smth_skew( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  faint::coord skew;
  if ( !parse_coord( args, &skew ) ){
    return 0;
  }

  Object* obj = self->obj;
  Tri oldTri( obj->GetTri() );
  Tri newTri( skewed(oldTri, skew ) );

  faint::python_run_command( self->canvas,
    new TriCommand( obj, New(newTri), Old(oldTri) ) );
  return Py_BuildValue("");
}

static PyObject* Smth_get_angle( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  Tri t = self->obj->GetTri();
  return build_radian( t.Angle() );
}

static PyObject* Smth_get_skew( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  Tri t = self->obj->GetTri();
  return build_coord(t.Skew());
}

static PyObject* Smth_pos(smthObject* self){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  return build_point(self->obj->GetTri().P0());
}

static PyObject* Smth_set_linewidth(smthObject* self, PyObject* args){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  if ( !self->obj->GetSettings().Has( ts_LineWidth ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support line width.");
    return 0;
  }

  faint::coord lineWidth;
  if ( !parse_coord(args, &lineWidth) ){
    return 0;
  }

  faint::python_run_command( self->canvas, new CmdChangeSetting<FloatSetting>( self->obj, ts_LineWidth, lineWidth ) );
  return Py_BuildValue("");
}

static PyObject* Smth_get_linewidth(smthObject* self, PyObject*){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  const Settings& settings = self->obj->GetSettings();
  if ( !settings.Has( ts_LineWidth ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support line width.");
    return 0;
  }
  return build_coord(settings.Get( ts_LineWidth ));
}

static PyObject* Smth_get_settings(smthObject* self, PyObject*){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  return pythoned( self->obj->GetSettings() );
}

static PyObject* Smth_set_linestyle(smthObject* self, PyObject* args){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  char* s_value;
  if ( !PyArg_ParseTuple( args, "s", &s_value ) ){
    return 0;
  }

  int i_value = -1;
  if ( strcmp(s_value, "solid" ) == 0 || strcmp(s_value, "s" ) == 0 ){
    i_value = LineStyle::SOLID;
  }
  else if ( strcmp(s_value, "long_dash" ) == 0 || strcmp(s_value, "ld" ) == 0 ){
    i_value = LineStyle::LONG_DASH;
  }
  else {
    PyErr_SetString( PyExc_ValueError, "Invalid string. Valid options are \"solid\" (or \"s\") and \"long_dash\" (or \"ld\")" );
    return 0;
  }

  faint::python_run_command( self->canvas, new CmdChangeSetting<IntSetting>( self->obj, ts_LineStyle, i_value ) );
  return Py_BuildValue("");
}

static PyObject* get_color_helper( smthObject* self, const ColorSetting& colorId ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  const Settings& settings = self->obj->GetSettings();
  if ( !settings.Has( colorId ) ){
    PyErr_SetString(PyExc_ValueError,
      colorId == ts_FgCol ?
      "That object does not support foreground color." :
      "That object does not support background color." );
    return 0;
  }

  return build_color_tuple( settings.Get( colorId ) );
}

static PyObject* set_color_helper( smthObject* self, PyObject* args, const ColorSetting& colorId ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  if ( !self->obj->GetSettings().Has( colorId ) ){
    PyErr_SetString(PyExc_ValueError,
      colorId == ts_FgCol ?
      "That object does not support foreground color." :
      "That object does not support background color." );
    return 0;
  }
  faint::Color color;
  if ( !parse_color( args, color ) ){
    return 0;
  }
  faint::python_run_command( self->canvas, new CmdChangeSetting<ColorSetting>( self->obj, colorId, color ) );
  return Py_BuildValue("");
}

static PyObject* Smth_set_fgcol( smthObject* self, PyObject* args ){
  return set_color_helper( self, args, ts_FgCol );
}

static PyObject* Smth_set_bgcol( smthObject* self, PyObject* args ){
  return set_color_helper( self, args, ts_BgCol );
}

static PyObject* Smth_get_bgcol( smthObject* self ){
  return get_color_helper( self, ts_BgCol );
}

static PyObject* Smth_get_fgcol( smthObject* self ){
  return get_color_helper( self, ts_FgCol );
}


static PyObject* Smth_get_linestyle( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  const Settings& settings = self->obj->GetSettings();
  if ( !settings.Has( ts_LineStyle ) ){
    PyErr_SetString( PyExc_ValueError, "This object does not have a linestyle attribute." );
    return Py_BuildValue("");
  }

  int style = settings.Get( ts_LineStyle );
  if ( style == LineStyle::SOLID ){
    return Py_BuildValue("s", "solid");
  }
  else if ( style == LineStyle::LONG_DASH ){
    return Py_BuildValue("s", "long_dash");
  }
  else {
    return Py_BuildValue("");
  }
}

static PyObject* Smth_rect(smthObject* self){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  return build_rect(self->obj->GetRect());
}

static PyObject* Smth_set_text(smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  ObjText* text = dynamic_cast<ObjText*>( self->obj );
  if (!text){
    std::string err = std::string(self->obj->GetType()) + " does not support text.";
    PyErr_SetString( PyExc_ValueError, err.c_str() );
    return 0;
  }

  char* str;
  if ( !PyArg_ParseTuple( args, "s", &str ) ){
    return 0;
  }

  faint::python_run_command( self->canvas, new
    TextEntryCommand( text, New(faint::utf8_string(str)), Old(text->GetString())) );
  return Py_BuildValue("");
}

static PyObject* Smth_get_text( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  ObjText* text = dynamic_cast<ObjText*>( self->obj );
  if (!text){
    std::string err = std::string(self->obj->GetType()) + " does not support text.";
    PyErr_SetString( PyExc_ValueError, err.c_str() );
    return 0;
  }

  TextBuffer* tb = text->GetTextBuffer();
  const char* str = tb->get().c_str();
  return Py_BuildValue( "s", str );
}

static PyObject* Smth_text_anchor_middle( smthObject* self, PyObject* ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  ObjText* text = dynamic_cast<ObjText*>( self->obj );
  if (!text){
    std::string err = std::string(self->obj->GetType()) + " does not support text.";
    PyErr_SetString( PyExc_ValueError, err.c_str() );
    return 0;
  }
  text->AnchorMiddle();
  return Py_BuildValue("");
}

static PyObject* Smth_get_png_string( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  ObjRaster* raster = dynamic_cast<ObjRaster*>( self->obj );
  if ( !raster ){
    std::string err = std::string(self->obj->GetType()) + " does not support images.";
    PyErr_SetString( PyExc_ValueError, err.c_str() );
    return 0;
  }

  std::string png( to_png_string( raster->GetBitmap() ) );
  PyObject* o_str = PyString_FromStringAndSize( png.c_str(), png.size() );

  return o_str;
}

static PyObject* Smth_get_text_lines( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  ObjText* text = dynamic_cast<ObjText*>( self->obj );
  if (!text){
    std::string err = std::string(self->obj->GetType()) + " does not support text.";
    PyErr_SetString( PyExc_ValueError, err.c_str() );
    return 0;
  }

  text_lines_t lines = split_text_buffer( *(text->GetTextBuffer()), text->GetTri().Width(), text->GetSettings() );
  PyObject* list = PyList_New( lines.size() );

  for ( size_t i = 0; i != lines.size(); i++ ){
    int hardBreak = lines[i].first ? 1 : 0;
    const faint::utf8_string& str( lines[i].second );
    PyObject* u_str = PyUnicode_FromString( str.c_str() );
    PyObject* tuple = Py_BuildValue("(iO)", hardBreak, u_str );
    PyList_SetItem( list, i, tuple );
  }
  return list;
}

static PyObject* Smth_get_text_height( smthObject* self, PyObject* ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  ObjText* txt = dynamic_cast<ObjText*>( self->obj );
  if ( txt == 0 ){
    PyErr_SetString( PyExc_ValueError, "This object does not have text attributes" );
    return 0;
  }

  return build_coord(txt->RowHeight());
}

static PyObject* Smth_get_text_line_spacing( smthObject* self, PyObject* ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  ObjText* txt = dynamic_cast<ObjText*>( self->obj );
  if ( txt == 0 ){
    PyErr_SetString( PyExc_ValueError, "This object does not have text attributes" );
    return 0;
  }

  return build_coord(txt->LineSpacing());
}

static PyObject* Smth_get_fillstyle( smthObject* self, PyObject* ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  const Settings& settings = self->obj->GetSettings();
  if ( !settings.Has( ts_FillStyle ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support fill styles");
    return 0;
  }

  const int fillStyle = settings.Get( ts_FillStyle );
  if ( fillStyle == FillStyle::BORDER ){
    return Py_BuildValue("s", "border" );
  }
  else if ( fillStyle == FillStyle::BORDER_AND_FILL ){
    return Py_BuildValue("s", "border+fill" );
  }
  else if ( fillStyle == FillStyle::FILL ){
    return Py_BuildValue("s", "fill" );
  }
  else {
    return Py_BuildValue("");
  }
}

static PyObject* Smth_get_transparency( smthObject* self, PyObject* ){
  // Fixme: These must be generated!
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  const Settings& settings = self->obj->GetSettings();
  if ( !settings.Has( ts_BackgroundStyle ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support a background style.");
    return 0;
  }

  const int transparent = settings.Get( ts_BackgroundStyle );
  if ( transparent == BackgroundStyle::MASKED ){
    return Py_BuildValue("s", "masked");
  }
  return Py_BuildValue("s", "opaque");
}

static PyObject* Smth_get_arrowhead( smthObject* self, PyObject* ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  const Settings& settings = self->obj->GetSettings();
  if ( !settings.Has( ts_LineArrowHead ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support arrowheads.");
    return 0;
  }
  const int arrowhead = settings.Get( ts_LineArrowHead );
  return Py_BuildValue("i", arrowhead );
}

static PyObject* Smth_set_fillstyle( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  if ( ! self->obj->GetSettings().Has( ts_FillStyle ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support fill styles");
    return 0;
  }

  char* s_value;
  if ( !PyArg_ParseTuple( args, "s", &s_value ) ){
    return 0;
  }

  if ( strcmp(s_value, "fill" ) == 0 || strcmp(s_value, "f") == 0 ){
    faint::python_run_command( self->canvas,
      new CmdChangeSetting<IntSetting>( self->obj, ts_FillStyle, FillStyle::FILL ) );
  }
  else if ( strcmp(s_value, "border" ) == 0 || strcmp(s_value, "b") == 0 ){
    faint::python_run_command( self->canvas,
      new CmdChangeSetting<IntSetting>( self->obj, ts_FillStyle, FillStyle::BORDER ) );
  }
  else if ( strcmp(s_value, "border+fill" ) == 0 || strcmp(s_value,"bf") == 0 || strcmp(s_value, "fb") == 0 ){
    faint::python_run_command( self->canvas,
      new CmdChangeSetting<IntSetting>( self->obj, ts_FillStyle, FillStyle::BORDER_AND_FILL ) );
  }
  else {
    PyErr_SetString( PyExc_ValueError, "Invalid fill-style specified. Valid options are \"border\" (or \"b\"), \"fill\" (or \"f\") or \"border+fill\" (or \"bf\")" );
    return 0;
  }
  return Py_BuildValue("");
}

static PyObject* Smth_get_fontsize( smthObject* self, PyObject* ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  if ( !self->obj->GetSettings().Has( ts_FontSize ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support fonts");
    return 0;
  }

  const Settings& settings = self->obj->GetSettings();
  return Py_BuildValue("i", settings.Get( ts_FontSize ) );
}

static PyObject* Smth_set_font( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  ObjText* textObj = dynamic_cast<ObjText*>( self->obj );
  if ( textObj == 0 ){
    PyErr_SetString(PyExc_ValueError, "That object does not support fonts");
    return 0;
  }

  char* s_value;
  if ( !PyArg_ParseTuple( args, "s", &s_value ) ){
    return 0;
  }
  faint::python_run_command( self->canvas, new CmdChangeSetting<StrSetting>( self->obj, ts_FontFace, s_value ) );
  return Py_BuildValue("");
}

static PyObject* Smth_get_font( smthObject* self, PyObject* ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  ObjText* textObj = dynamic_cast<ObjText*>( self->obj );
  if ( textObj == 0 ){
    PyErr_SetString(PyExc_ValueError, "That object does not support fonts");
    return 0;
  }

  const std::string font = textObj->GetSettings().Get( ts_FontFace );
  return Py_BuildValue("s", font.c_str() );
}

static PyObject* Smth_set_fontsize( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  int size;
  if ( !PyArg_ParseTuple( args, "i", &size ) ){
    return 0;
  }
  if ( !self->obj->GetSettings().Has( ts_FontSize ) ){
    PyErr_SetString(PyExc_ValueError, "That object does not support fonts");
    return 0;
  }

  faint::python_run_command( self->canvas, new CmdChangeSetting<IntSetting>( self->obj, ts_FontSize, size ) );
  return Py_BuildValue("");
}

static PyObject* Smth_getObjCount( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  ObjComposite* composite = dynamic_cast<ObjComposite*>(self->obj);
  if ( composite == 0 ){
    PyErr_SetString( PyExc_ValueError, "This object does not support sub-objects" );
    return 0;
  }

  return Py_BuildValue("i", composite->GetObjectCount() );
}

static PyObject* Smth_getSubObject( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  int num = 0;
  if ( !PyArg_ParseTuple( args, "i", &num) ){
    PyErr_SetString( PyExc_ValueError, "Invalid parameter" );
    return 0;
  }

  ObjComposite* composite = dynamic_cast<ObjComposite*>(self->obj);
  if ( composite == 0 ){
    PyErr_SetString( PyExc_ValueError, "This object does not support sub-objects" );
    return 0;
  }
  return pythoned( composite->GetObject( num ), self->canvas );
}

static PyObject* Smth_pointList(smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  if ( self->obj->GetType() == s_TypePolygon ){
    ObjPolygon* polygon = dynamic_cast<ObjPolygon*>( self->obj );
    const std::vector<Point> points = polygon->Vertices();
    PyObject* list = PyList_New( points.size() * 2 );

    for ( size_t i = 0; i != points.size(); i++ ){
      faint::PyPoint pt = point_to_py( points[i] );
      PyList_SetItem( list, i * 2, pt.x );
      PyList_SetItem( list, i * 2 + 1, pt.y );
    }
    return list;
  }
  else if ( self->obj->GetType() == s_TypeSpline ) {
    ObjSpline* spline = dynamic_cast<ObjSpline*>( self->obj );
    std::vector<Point> points = spline->GetSplinePoints();
    PyObject* list = PyList_New( points.size() * 2 );
    for ( size_t i = 0; i != points.size(); i++ ){
      faint::PyPoint pt = point_to_py( points[i] );
      PyList_SetItem( list, i * 2, pt.x );
      PyList_SetItem( list, i * 2 + 1, pt.y );
    }
    return list;
  }
  else if ( self->obj->GetType() == s_TypeLine ){
    std::vector<Point> points = self->obj->GetMovablePoints();
    PyObject* list = PyList_New( points.size() * 2 );

    for ( size_t i = 0; i != points.size(); i++ ){
      faint::PyPoint pt = point_to_py( points[i] );
      PyList_SetItem( list, i * 2, pt.x );
      PyList_SetItem( list, i * 2 + 1, pt.y );
    }
    return list;
  }
  else {
    // Use the Tri-points to form a polygon
    PyObject* list = PyList_New( 8 );
    Tri tri = self->obj->GetTri();
    faint::PyPoint p0 = point_to_py(tri.P0());
    faint::PyPoint p1 = point_to_py(tri.P1());
    faint::PyPoint p2 = point_to_py(tri.P2());
    faint::PyPoint p3 = point_to_py(tri.P3());
    PyList_SetItem(list, 0, p0.x );
    PyList_SetItem(list, 1, p0.y );
    PyList_SetItem(list, 2, p1.x );
    PyList_SetItem(list, 3, p1.y );
    PyList_SetItem(list, 4, p3.x );
    PyList_SetItem(list, 5, p3.y );
    PyList_SetItem(list, 6, p2.x );
    PyList_SetItem(list, 7, p2.y );
    return list;
  }
}

static PyObject* Smth_get_type(smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
      PyErr_SetString(PyExc_ValueError, "Removed object");
  }
  return Py_BuildValue("s", self->obj->GetType() );
}

static PyObject* Smth_tri( smthObject* self ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }
  return pythoned( self->obj->GetTri() );
}

static PyObject* Smth_set_tri( smthObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) || !self->canvas->Has( self->objectId ) ){
    PyErr_SetString(PyExc_ValueError, "That object is removed.");
    return 0;
  }

  triObject* tri;
  if ( !PyArg_ParseTuple( args, "O!", &TriType, &tri ) ){
    return 0;
  }
  faint::python_run_command( self->canvas, new TriCommand( self->obj, New(tri->tri), Old(self->obj->GetTri() ) ));
  return Py_BuildValue("");
}

// Fixme: Generate all setting-setters. Merge with self-defined functions for like retrieving sub objects etc.
static PyMethodDef smth_methods[] = {
  {"moveto", (PyCFunction)Smth_moveto, METH_VARARGS,
   "Move the thing"},
  {"pos", (PyCFunction)Smth_pos, METH_NOARGS,
   "Get the position as a tuple, (x,y)."},
  {"rect", (PyCFunction)Smth_rect, METH_NOARGS,
   "Get the bounding rectangle as a tuple, (x,y, width, height)."},
  {"rect", (PyCFunction)Smth_rect, METH_NOARGS,
   "Get the bounding rectangle as a tuple, (x,y, width, height)."},
  {"tri", (PyCFunction)Smth_tri, METH_NOARGS, "tri()->Tri\nRetrieve a Tri describing the position, skew and angle of the object."},
  {"set_tri", (PyCFunction)Smth_set_tri, METH_VARARGS, "set_tri(tri)\nAdjust this object's geometry to the passed in triangle."},
  {"set_linewidth", (PyCFunction)Smth_set_linewidth, METH_VARARGS,
   "Set the width of the edge line"},
  {"get_linewidth", (PyCFunction)Smth_get_linewidth, METH_VARARGS,
   "Retrieve the width of the edge line"},
  {"set_linestyle", (PyCFunction)Smth_set_linestyle, METH_VARARGS,
   "Set the style of the line"},
  {"get_linestyle", (PyCFunction)Smth_get_linestyle, METH_NOARGS,
   "Get the style of the line"},
  {"set_text", (PyCFunction)Smth_set_text, METH_VARARGS,
   "set_text(s)\nSet the text of Text-objects"},
  {"get_png_string", (PyCFunction)Smth_get_png_string, METH_NOARGS,
   "get_png_string()->s\nReturns a string of the bitmap encoded as PNG (Raster-objects only)"},
  {"get_settings", (PyCFunction)Smth_get_settings, METH_NOARGS, "get_settings()-> settings\nReturns the settings of the object."},
  {"get_text", (PyCFunction)Smth_get_text, METH_NOARGS, "get_text()-> s\nReturns the text from Text-objects."},
  {"get_text_lines", (PyCFunction)Smth_get_text_lines, METH_NOARGS, "get_text_lines()->[s,...]\nReturns the text split into lines from Text-objects."},
  {"text_anchor_middle", (PyCFunction)Smth_text_anchor_middle, METH_NOARGS, "Anchor the text at the center"},
  {"rotate", (PyCFunction)Smth_rotate, METH_VARARGS, "Rotate the thing"},
  {"skew", (PyCFunction)Smth_skew, METH_VARARGS, "Skew the thing"},
  {"get_skew", (PyCFunction)Smth_get_skew, METH_VARARGS, "Skew the thing"},
  {"get_angle", (PyCFunction)Smth_get_angle, METH_NOARGS, "Returns the angle of the object" },
  {"get_points", (PyCFunction)Smth_pointList, METH_NOARGS, "get_points()->(f,f...) returns a list of points for Polygons, Splines and Paths" },
  {"get_fgcol", (PyCFunction)Smth_get_fgcol, METH_NOARGS, "get_fgcol()->(r,g,b[,a])\nreturns the foreground color" },
  {"get_bgcol", (PyCFunction)Smth_get_bgcol, METH_NOARGS, "get_bgcol()->(r,g,b,[a]\nreturns the background color as a tuple" },
  {"set_fgcol", (PyCFunction)Smth_set_fgcol, METH_VARARGS, "set_fgcol(r,g,b[,a])\nsets the foreground color." },
  {"set_bgcol", (PyCFunction)Smth_set_bgcol, METH_VARARGS, "set_bgcol(r,g,b[,a])\nsets the background color." },
  {"set_fillstyle", (PyCFunction)Smth_set_fillstyle, METH_VARARGS, "" },
  {"get_fillstyle", (PyCFunction)Smth_get_fillstyle, METH_VARARGS, "Returns the object's fill-style" },
  {"get_arrowhead", (PyCFunction)Smth_get_arrowhead, METH_VARARGS, "Returns the arrow-head setting" },
  {"set_font", (PyCFunction)Smth_set_font, METH_VARARGS, "Sets the font of the object"},
  {"get_font", (PyCFunction)Smth_get_font, METH_NOARGS, "Returns the object's font"},
  {"set_fontsize", (PyCFunction)Smth_set_fontsize, METH_VARARGS, "set_fontsize(i)\nSets the point-size of the object's font (for text objects only)" },
  {"get_fontsize", (PyCFunction)Smth_get_fontsize, METH_VARARGS, "get_fontsize()->i\nReturns the point-size of the object's font (for text objects only)" },
  {"get_transparency", (PyCFunction)Smth_get_transparency, METH_NOARGS, "Returns the object's transparency setting"},
  {"num_objs", (PyCFunction)Smth_getObjCount, METH_NOARGS, "num_objs()->i\nReturns the number of sub-objects. Only useful for groups." },
  {"get_obj", (PyCFunction)Smth_getSubObject, METH_VARARGS, "get_obj(i)-> Object\nReturns the sub-object specified by the passed in integer.\nOnly useful for groups. " },
  {"get_text_height", (PyCFunction)Smth_get_text_height, METH_NOARGS, "get_text_height()->f\nReturns the text height for this object.\nRelevant only for text objects."},
  {"get_text_line_spacing", (PyCFunction)Smth_get_text_line_spacing, METH_NOARGS, "get_text_line_spacing()->f\nReturns the text line spacing for this object.\nRelevant only for text objects."},
  {"get_type", (PyCFunction)Smth_get_type, METH_NOARGS, "get_type()->s\nReturns the type of the object (e.g. Rectangle, Ellipse, Line...)" },
  {"update_settings", (PyCFunction)Smth_update_settings, METH_VARARGS, "update_settings(s)\nUpdate this object's settings with those from the passed in\nSettings object.\nAny settings not relevant for this object will be ignored"},
  {0,0,0,0}  // Sentinel
};


static int Smth_init(smthObject* self, PyObject*, PyObject*){
  self->obj = 0;
  return 0;
}

static PyObject* Smth_new(PyTypeObject* type, PyObject* , PyObject* ){
  smthObject *self;
  self = (smthObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* Smth_repr(smthObject* self ){
  if ( !GetAppContext().Exists( self->canvasId ) ){
    return Py_BuildValue("s", "Orphaned object" );
  }
  else if ( !self->canvas->Has( self->objectId ) ){
    return Py_BuildValue("s", "Removed object" );
  }
  else {
    const char* s( self->obj->GetType() );
    return Py_BuildValue("s", s);
  }
}

PyTypeObject SmthType = {
  PyObject_HEAD_INIT(NULL)
  0, // ob_size
  "Something", // tp_name
  sizeof(smthObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)Smth_repr, // tp_repr
  0, // tp_as_number
  0, // tp_as_sequence
  0, // tp_as_mapping
  0, // tp_hash
  0, // tp_call
  0, // tp_str
  0, // tp_getattro
  0, // tp_setattro
  0, // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
  "The type for the Python-representation of Faint objects", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  smth_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)Smth_init, // tp_init
  0, // tp_alloc
  Smth_new, // tp_new
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
