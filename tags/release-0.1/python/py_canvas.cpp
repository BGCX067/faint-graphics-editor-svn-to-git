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

#include "pythoninclude.hh"
#include "bitmap/cairo_util.h"
#include "canvasinterface.hh"
#include "commands/addobjectcommand.hh"
#include "commands/cmdgroupobjects.hh"
#include "commands/cmdsetbitmap.hh"
#include "commands/functioncommand.hh"
#include "commands/resizecommand.hh"
#include "getappcontext.hh"
#include "objects/objraster.hh"
#include "objects/objtext.hh"
#include "objects/objtri.hh"
#include "py_canvas.hh"
#include "py_create_object.hh"
#include "py_util.hh"
#include "pyinterface.hh"
#include "save.hh"
#include "settings.hh"
#include "tools/ellipsebehavior.hh"
#include "tools/linebehavior.hh"
#include "tools/rectanglebehavior.hh"
#include "util/autocrop.hh"
#include "util/commandutil.hh"
#include "util/defaultsettings.hh"
#include "util/pathutil.hh"

std::string to_png( const faint::Bitmap& ); // Fixme

bool CanvasOK( const CanvasId& c ){
  return GetAppContext().Exists( c );
}

PyObject* CanvasRemovedError(){
  PyErr_SetString( PyExc_ValueError, "Operation attempted on closed canvas." );
  return 0;
}

static PyObject* canvas_undo(canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  self->canvas->Undo();
  return Py_BuildValue( "" );
}

static PyObject* canvas_redo(canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  self->canvas->Redo();
  return Py_BuildValue("");
}

static PyObject* canvas_context_crop( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  self->canvas->ContextCrop();
  return Py_BuildValue( "" );
}

PyObject* canvas_context_flatten( canvasObject* self, PyObject* /*args*/ ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  self->canvas->ContextFlatten();
  return Py_BuildValue("");
}

static PyObject* canvas_close(canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  GetAppContext().Close( *( self->canvas ) );
  return Py_BuildValue("");
}

static PyObject* canvas_context_flip_horizontal(canvasObject* self, PyObject* ){
  if ( !CanvasOK(self->id ) ){
    return CanvasRemovedError();
  }
  self->canvas->ContextFlipHorizontal();
  return Py_BuildValue("");
}

static PyObject* canvas_context_flip_vertical(canvasObject* self, PyObject* ){
  if ( !CanvasOK(self->id ) ){
    return CanvasRemovedError();
  }
  self->canvas->ContextFlipVertical();
  return Py_BuildValue("");
}

static PyObject* canvas_context_rotate90CW(canvasObject* self, PyObject* ){
  if ( !CanvasOK(self->id ) ){
    return CanvasRemovedError();
  }
  self->canvas->ContextRotate90CW();
  return Py_BuildValue("");
}

bool GetPixel( const faint::Bitmap& bmp, const IntPoint& pt, faint::Color& color ){
  if ( pt.x < 0
    || pt.y < 0
    || pt.x >= static_cast<int>(bmp.m_w)
    || pt.y >= static_cast<int>(bmp.m_h) ) {
    return false;
  }
  color = GetColor( bmp, pt );
  return true;
}

static PyObject* canvas_get_background_png_string( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  const faint::Bitmap& bmp = self->canvas->GetImage().GetBitmapRef();
  std::string pngString( to_png( bmp ) );
  PyObject* pythonString = PyString_FromStringAndSize( pngString.c_str(), pngString.size() );
  return pythonString;
}

PyObject* canvas_center_view( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  int x, y;
  if ( !PyArg_ParseTuple( args, "ii", &x, &y ) ){
    return 0;
  }
  CanvasInterface& canvas = GetAppContext().GetActiveCanvas();
  canvas.CenterView( Point( x, y ) );
  return Py_BuildValue("");
}

static PyObject* canvas_get_pixel( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  int x, y;
  if ( !PyArg_ParseTuple( args, "ii", &x, &y ) ){
    return 0;
  }
  faint::Bitmap& bmp = self->canvas->GetImage().GetBitmapRef();
  faint::Color color;
  if ( GetPixel(bmp, IntPoint( x, y ), color ) ){
    return Py_BuildValue("(iiii)", color.r, color.g, color.b, color.a );
  }

  PyErr_SetString( PyExc_ValueError, "Failed retrieving color" );
  return 0;
}

static PyObject* canvas_set_pixel( canvasObject* self, PyObject* args ){
  int x, y, r, g, b, a;
  if ( !PyArg_ParseTuple( args, "ii(iiii)", &x, &y, &r, &g, &b, &a ) ){
    PyErr_Clear();
    a = 255;
    if ( !PyArg_ParseTuple( args, "ii(iii)", &x, &y, &r, &g, &b ) ){
      return 0;
    }
  }
  if ( invalid_color(r,g,b,a) ){
    return 0;
  }

  faint::Bitmap& bmp = self->canvas->GetBitmap();
  if ( invalid_pixel_pos(x, y, bmp) ){
    return 0;
  }

  // Fixme: Need a command. Not one per pixel though...
  faint::PutPixel( bmp, x,y, faint::ColorFromInts( r, g, b, a ) );
  return Py_BuildValue("");
}

void grid_off( canvasObject* self ){
  Grid g( self->canvas->GetGrid() );
  g.SetEnabled( false );
  g.SetVisible( false );
  self->canvas->SetGrid( g );
  self->canvas->Refresh();
}

static PyObject* canvas_set_grid(canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  int size = 0;
  if ( !PyArg_ParseTuple(args, "i", &size ) ){
    return 0;
  }
  if ( size < 0 ){
    PyErr_SetString( PyExc_ValueError, "Grid size must be a positive number." );
    return 0;
  }
  if ( size == 0 ){
    grid_off( self );
    return Py_BuildValue("");
  }

  self->canvas->SetGrid( Grid(true, true, size ) );
  self->canvas->Refresh();
  return Py_BuildValue( "" );
}

static PyObject* canvas_get_grid(canvasObject* self){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  Grid g = self->canvas->GetGrid();
  int size = g.Spacing();
  return Py_BuildValue("i", size);
}

static PyObject* canvas_set_grid_off(canvasObject* self){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  grid_off( self );
  return Py_BuildValue("");
}

static PyObject* canvas_grid_on(canvasObject* self){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  Grid g(self->canvas->GetGrid() );
  return PythonBool(g.Enabled());
}

static PyObject* canvas_set_grid_on(canvasObject* self){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  Grid g( self->canvas->GetGrid() );
  g.SetEnabled( true );
  g.SetVisible( true );
  self->canvas->SetGrid(g );
  self->canvas->Refresh();
  return Py_BuildValue( "" );
}

static PyObject* canvas_save(canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  if ( PySequence_Length( args ) == 0 ){
    // No filename specified, use the current filename
    std::string filename = self->canvas->GetFilename();
    if ( filename.empty() ){
      PyErr_SetString( PyExc_ValueError, "No filename specified, and no previously used filename available." );
      return 0;
    }
    if ( faint::Save( *(self->canvas), filename ) ){
      self->canvas->SetFilename( filename );
      self->canvas->ClearDirty();
      return Py_BuildValue("");
    }
    else {
      PyErr_SetString( PyExc_ValueError, "Failed saving"  );
    }
  }

  // Use the filename passed as argument
  char* filename = 0;
  if ( !PyArg_ParseTuple( args, "s", &filename ) ){
    return 0;
  }
  if ( !ValidSaveFileName( filename ) ){
    PyErr_SetString( PyExc_ValueError, "Invalid file name" );
    return 0;
  }

  bool saveOK = faint::Save( *(self->canvas), filename );
  if ( !saveOK ){
    PyErr_SetString( PyExc_ValueError, "Failed saving" );
    return 0;
  }
  self->canvas->SetFilename( filename );
  self->canvas->ClearDirty();
  return Py_BuildValue("");
}

static PyObject* canvas_select_object( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  int n = PySequence_Length( args );
  if ( n == 0 ){
    self->canvas->DeselectObjects();
    return Py_BuildValue("");
  }

  PyObject* sequence = args;

  if ( n == 1 ){
    PyObject* first = PySequence_GetItem( sequence, 0 );
    if ( PySequence_Check( first ) ){
      sequence = first;
      n = PySequence_Length( sequence );
    }
  }

  std::vector<smthObject*> objects;
  for ( int i = 0; i != n; i++ ){
    PyObject* item = PySequence_GetItem( sequence, i );
    smthObject* obj = as_smthObject( item );
    if ( obj == 0 ){
      PyErr_SetString(PyExc_ValueError, "That's not a Faint object.");
      return 0;
    }
    if ( !self->canvas->Has( obj->objectId ) ){
      PyErr_SetString(PyExc_ValueError, "The canvas does not contain that item.");
      return 0;
    }
    objects.push_back(obj);
  }

  self->canvas->DeselectObjects();
  for ( size_t i = 0; i != objects.size(); i++ ){
    self->canvas->SelectObject( objects[i]->obj, false );
  }
  return Py_BuildValue("");
}

static PyObject* canvas_save_backup( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  if ( PySequence_Length( args ) == 0 ){
    PyErr_SetString( PyExc_ValueError, "No backup filename specified" );
    return 0;
  }

  char* filename = 0;
  if ( PyArg_ParseTuple( args, "s", &filename ) ){
    if ( !ValidSaveFileName( filename ) ){
      PyErr_SetString( PyExc_ValueError, "Invalid file name" );
      return 0;
    }

    bool saveOK = faint::Save( *(self->canvas), filename );
    if ( !saveOK ){
      PyErr_SetString( PyExc_ValueError, "Failed saving" );
      return 0;
    }
    return Py_BuildValue("");
  }
  return 0;
}

template<Object* (*Creator)(PyObject*)>
static PyObject* canvas_add_object(canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  Object* obj = Creator( args );
  if ( obj == 0 ){
    return 0;
  }

  PythonRunCommand( self->canvas, new AddObjectCommand(obj) );  
  return Pythoned(obj, self->canvas);
}

static PyObject* canvas_line(canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  faint::coord x0, y0, x1, y1;
  if ( !PyArg_ParseTuple( args, "dddd", &x0, &y0, &x1, &y1 ) ){ // Fixme: Hide d
    PyErr_SetString( PyExc_ValueError, "Invalid parameters to line( x0, y0, x1, y1 )" );
    return 0;
  }

  FaintSettings s = GetLineSettings();
  s.Set( ts_AntiAlias, false );
  s.Update( GetAppContext().GetToolSettings() );
  Command* cmd = new LineCommand( Point(x0, y0), Point( x1, y1 ), s );
  PythonRunCommand(self->canvas, cmd);
  return Py_BuildValue("");
}

static PyObject* canvas_rect(canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  faint::coord x, y, w, h;
  if ( !PyArg_ParseTuple( args, "dddd", &x, &y, &w, &h ) ){ // Fixme: Hide "d"
    PyErr_SetString( PyExc_ValueError, "Invalid parameters to rect(x, y, w, h)" );
    return 0;
  }

  FaintSettings s = GetRectangleSettings();
  s.Update( GetAppContext().GetToolSettings() );
  s.Set( ts_AntiAlias, false );
  Command* cmd = new RectangleCommand( Point(x, y), Point( x + w - 1, y + h - 1 ), s );
  PythonRunCommand(self->canvas, cmd);
  return Py_BuildValue("");
}

static PyObject* canvas_ellipse(canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  double x, y, w, h;
  if ( !PyArg_ParseTuple( args, "dddd", &x, &y, &w, &h ) ){ // Fixme: Hide d
    PyErr_SetString( PyExc_ValueError, "Invalid parameters to ellipse(x, y, w, h)" );
    return 0;
  }

  FaintSettings s = GetEllipseSettings();
  s.Update( GetAppContext().GetToolSettings() );
  s.Set( ts_AntiAlias, false );
  Command* cmd = new EllipseCommand( IntRect(IntPoint(x, y), IntPoint(w, h)), s );
  PythonRunCommand(self->canvas, cmd);
  return Py_BuildValue("");
}

static PyObject* canvas_obj_line(canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateLine>(self,args);
}

static PyObject* canvas_obj_tri(canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  double x0, y0, x1, y1, x2, y2;
  if ( !PyArg_ParseTuple( args, "dddddd", &x0, &y0, &x1, &y1, &x2, &y2 ) ){
    return 0;
  }

  FaintSettings settings( GetLineSettings() );
  settings.Update( GetAppContext().GetToolSettings() );
  ObjTri* objTri = new ObjTri( Tri(Point(x0,y0), Point(x1,y1), Point(x2,y2)), settings );
  PythonRunCommand( self->canvas, new AddObjectCommand( objTri ) );
  return Pythoned( objTri, self->canvas );;
}

static PyObject* canvas_obj_rect(canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateRectangle>(self,args);
}

static PyObject* canvas_obj_raster( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateRaster>(self,args);
}

PyObject* canvas_obj_ellipse( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateEllipse>(self, args);
}

static PyObject* canvas_obj_polygon( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreatePolygon>(self, args);
}

static PyObject* canvas_obj_group( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  // Prevent empty groups
  if ( PySequence_Length( args ) == 0 ){
    PyErr_SetString( PyExc_ValueError, "A group must contain at least one object." );
    return 0;
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
    PyErr_SetString( PyExc_ValueError, "A group must contain at least one object." );
    return 0;
  }

  std::vector<Object*> faintObjects;
  for ( int i = 0; i != n; i++ ){
    PyObject* object = PySequence_GetItem( sequence, i );
    if ( !PyObject_IsInstance( object, (PyObject*)&SmthType ) ){
      PyErr_SetString( PyExc_ValueError, "Unsupported item in list" );
      return 0;
    }
    faintObjects.push_back( ((smthObject*)object)->obj );
  }

  GroupObjects* cmd = new GroupObjects( faintObjects );
  PythonRunCommand( self->canvas, cmd );
  return Pythoned( cmd->GetComposite(), self->canvas );
}

static PyObject* canvas_obj_spline( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateSpline>(self,args);
}

static PyObject* canvas_obj_path( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreatePath>(self,args);
}

bool to_wxString( PyObject* str_py, wxString& str_wx ){
  if ( PyUnicode_Check( str_py ) ){
    size_t len = PyUnicode_GET_SIZE( str_py );
    PyUnicode_AsWideChar( (PyUnicodeObject*)str_py, wxStringBuffer(str_wx, len),len );
    return true;
  }
  else if ( PyString_Check( str_py ) ) {
    const char* c_str = PyString_AS_STRING(str_py);
    str_wx = wxString(c_str);
    return true;
  }
  PyErr_SetString(PyExc_TypeError, "string or unicode type required." );
  return false;
}

static PyObject* canvas_obj_text( canvasObject* self, PyObject* args ){
  return canvas_add_object<CreateText>(self, args);
}

static PyObject* canvas_get_objects( canvasObject* self ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  std::vector<Object*>& objects = self->canvas->GetObjects();
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i!= objects.size(); i++ ){
    PyList_Append( py_list, Pythoned( objects[i], self->canvas ) );
  }
  return py_list;
}

static PyObject* canvas_get_mouse_pos( canvasObject* self ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  IntPoint p( truncated(self->canvas->GetRelativeMousePos()) );
  return Py_BuildValue("ii", p.x, p.y);
}

PyObject* canvas_get_selected_objects( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  std::vector<Object*>& objects = self->canvas->GetSelectedObjects();
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i!= objects.size(); i++ ){
    PyObject* smth = Pythoned(objects[i], self->canvas);
    PyList_Append( py_list, smth );
  }
  return py_list;
}


PyObject* canvas_get_selection( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  IntRect r = self->canvas->GetRasterSelection();
  if ( r.w == 0 || r.h == 0 ){
    return Py_BuildValue("");
  }
  return Py_BuildValue("iiii", r.x, r.y, r.w, r.h );
}

PyObject* canvas_set_selection( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  int x, y, w, h;
  // Try parsing as normal xywh parameters
  if ( !PyArg_ParseTuple( args, "iiii", &x, &y, &w, &h ) ){
    PyErr_Clear();
    // Try parsing as a single tuple
    if ( !PyArg_ParseTuple( args, "(iiii)", &x, &y, &w, &h ) ){
      return 0;
    }
  }

  if ( x < 0 || y < 0 ){
    PyErr_SetString( PyExc_ValueError, "The x and y coordinates of the selection rectangle must be positive." );
    return 0;
  }
  if ( w <= 0 || h <= 0 ){
    PyErr_SetString( PyExc_ValueError, "The width and height of the selection rectangle must be greater than zero.");
    return 0;
  }

  self->canvas->SetRasterSelection( IntRect( IntPoint(x, y), IntSize(w, h) ) );
  self->canvas->Refresh(); // Fixme: Refresh should be delayed, but
                           // this only works for commands at the
                           // moment
  return Py_BuildValue("");
}

PyObject* canvas_shrink_selection( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  IntRect selection(self->canvas->GetRasterSelection());
  if ( Empty( selection ) ){
    IntRect cropRect;
    if ( GetAutoCropRect( self->canvas->GetBitmap(), cropRect ) ){
      self->canvas->SetRasterSelection( Translated( cropRect, selection.TopLeft() ) );
    }
  }
  else {
    faint::Bitmap bmpSelected( CairoCompatibleSubBitmap( self->canvas->GetBitmap(), selection ) );
    IntRect cropRect;
    if ( GetAutoCropRect( bmpSelected, cropRect ) ){
      self->canvas->SetRasterSelection( Translated( cropRect, selection.TopLeft() ) );
    }
  }
  self->canvas->Refresh(); // Fixme: Refresh should be delayed, but
                           // this only works for commands at the
                           // moment
  return Py_BuildValue("");
}


static PyObject* canvas_filename( canvasObject* self ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  std::string filename = self->canvas->GetFilename();
  if ( filename.size() == 0 ){
    return Py_BuildValue("");
  }
  return Py_BuildValue( "s", filename.c_str() );
}

static PyObject* canvas_get_id( canvasObject* self ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  return Py_BuildValue( "i", self->id.Raw() );
}

static PyObject* canvas_set_rect( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  int x, y, w, h;
  // Try parsing as normal xywh parameters
  if ( !PyArg_ParseTuple( args, "iiii", &x, &y, &w, &h ) ){
    PyErr_Clear();
    // Try parsing as a single tuple
    if ( !PyArg_ParseTuple( args, "(iiii)", &x, &y, &w, &h ) ){
      return 0;
    }
  }

  faint::Color bgCol = GetAppContext().GetToolSettings().Get( ts_BgCol );
  Command* resize = new ResizeCommand( IntRect( IntPoint(x, y), IntSize(w, h) ), bgCol  );
  PythonRunCommand(self->canvas, resize);
  return Py_BuildValue("");
}

static PyObject* canvas_set_size( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  const int n = PySequence_Length( args );
  if ( n < 3 ){
    // No color specified
    int w = 0;
    int h = 0;
    if ( !PyArg_ParseTuple( args, "ii", &w, &h ) ){
      PyErr_SetString(PyExc_ValueError, "Two numeric arguments required");
      return 0;
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString(PyExc_ValueError, "Negative or zero width or height argument");
      return 0;
    }
    faint::Color bgCol = GetAppContext().GetToolSettings().Get( ts_BgCol );
    PythonRunCommand( self->canvas, new ResizeCommand( IntSize(w, h), bgCol ) );
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
      PyErr_SetString(PyExc_ValueError, "Two numeric arguments and and an RGB- or RGBA-color tuple is required.");
      return 0;
    }
    a = 255;
  }

  if ( w <= 0 || h <= 0 ){
    PyErr_SetString(PyExc_ValueError, "Negative or zero width or height argument.");
    return 0;
  }

  if ( invalid_color( r, g, b, a ) ){
    return 0;
  }

  PythonRunCommand( self->canvas,
    new ResizeCommand( IntSize( w, h ), faint::Color( r, g, b, a ) ) );
  return Py_BuildValue("");
}

static PyObject* canvas_get_size( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  IntSize size( self->canvas->GetSize() );
  return Py_BuildValue("ii", size.w, size.h);
}

static PyObject* canvas_desaturate( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  PythonRunCommand(self->canvas,
    new FunctionCommand<faint::DesaturateSimple> );
  return Py_BuildValue("");
}

static PyObject* canvas_desaturate_weighted( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  PythonRunCommand(self->canvas, new
    FunctionCommand<faint::DesaturateWeighted> );
  return Py_BuildValue("");
}

PyObject* canvas_invert( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  PythonRunCommand( self->canvas,
    new FunctionCommand<faint::Invert> );
  return Py_BuildValue("");
}

PyObject* canvas_replace_color( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  if ( PySequence_Length( args ) != 2 ){
    PyErr_SetString( PyExc_ValueError, "Invalid number of arguments");
  }

  faint::Color color1, color2;
  bool colorsOk = ParseColor( PySequence_GetItem( args, 0 ), color1 ) &&
    ParseColor( PySequence_GetItem( args, 1 ), color2 );

  if ( !colorsOk ){
    return 0;
  }

  typedef FunctionCommand2<faint::Color, faint::Color, faint::ReplaceColor > ReplaceColorCommand;
  PythonRunCommand( self->canvas,
    new ReplaceColorCommand( color1, color2 ) );
  return Py_BuildValue("");
}

PyObject* canvas_replace_alpha( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  faint::Color c;
  if ( !ParseColorRGB( args, c ) ){
    return 0;
  }
  typedef FunctionCommand1<faint::Color, faint::BlendAlpha > BlendAlphaCommand;
  PythonRunCommand( self->canvas, new BlendAlphaCommand( c ) );
  return Py_BuildValue("");
}

static PyObject* canvas_context_delete( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ContextDelete();
  return Py_BuildValue("");
}

static PyObject* canvas_delete_objects( canvasObject* self, PyObject* args ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }

  std::vector<Object*> objects;
  if ( ! ObjectsFromArgs( args, objects ) ){
    return 0;
  }
  CanvasInterface* canvas = self->canvas;
  for ( size_t i = 0; i != objects.size(); i++ ){
    if ( !canvas->Has( objects[i]->GetId() ) ){
      PyErr_SetString(PyExc_ValueError, "Atleast one object is not in this canvas.");
      return 0;
    }
  }

  PythonRunCommand( canvas,
    GetDeleteObjectsCommand( objects, canvas->GetImage() ) );
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_page_left( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageLeft();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_page_right( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageRight();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_page_up( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageUp();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_page_down( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollPageDown();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_up( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxUp();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_down( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxDown();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_left( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxLeft();
  return Py_BuildValue("");
}

static PyObject* canvas_scroll_max_right( canvasObject* self, PyObject* ){
  if ( !CanvasOK( self->id ) ){
    return CanvasRemovedError();
  }
  CanvasInterface& canvas = *(self->canvas);
  canvas.ScrollMaxRight();
  return Py_BuildValue("");
}

// Method table for Python-Canvas interface (excluding standard
// functions like del, repr and str) Mind the per-section alphabetical
// ordering!
static PyMethodDef canvas_methods[] = {
  // Raster drawing methods
  {"line", (PyCFunction)canvas_line, METH_VARARGS,
   "line(x0,y0,x1,y1)\nDraw a line from x0,y0 to x1,y1"},
  {"rect", (PyCFunction)canvas_rect, METH_VARARGS,
   "rect(x,y,width,height)\nDraw a rectangle"},
  {"ellipse", (PyCFunction)canvas_ellipse, METH_VARARGS,
   "ellipse(x,y,width,height)\nDraw an ellipse"},

  // Add-object methods
  {"Ellipse", (PyCFunction)canvas_obj_ellipse, METH_VARARGS, "Adds an Ellipse object"},
  {"Group", (PyCFunction)canvas_obj_group, METH_VARARGS, "Adds a Group"},
  {"Line", (PyCFunction)canvas_obj_line, METH_VARARGS, "Adds a Line object"},
  {"Path", (PyCFunction)canvas_obj_path, METH_VARARGS, "Adds a Path object"},
  {"Polygon", (PyCFunction)canvas_obj_polygon, METH_VARARGS, "Adds a Polygon object"},
  {"Raster", (PyCFunction)canvas_obj_raster, METH_VARARGS, "Adds a Raster object"},
  {"Rect", (PyCFunction)canvas_obj_rect, METH_VARARGS, "Adds a Rectangle object"},
  {"ObjTri", (PyCFunction)canvas_obj_tri, METH_VARARGS, "Adds a Tri (debug) object"},
  {"Spline", (PyCFunction)canvas_obj_spline, METH_VARARGS, "Adds a Spline object"},
  {"Text", (PyCFunction)canvas_obj_text, METH_VARARGS, "Adds a Text object "},

  // Misc. methods
  {"center", (PyCFunction)canvas_center_view, METH_VARARGS,
   "center(x,y)\nCenters the view at image position (x,y)"},
  {"close", (PyCFunction)canvas_close, METH_NOARGS,
   "Close the canvas"},
  {"context_crop", (PyCFunction)canvas_context_crop, METH_NOARGS,
   "context_crop()\nCrops the image to the raster selection, crops a selected object or performs an autocrop."},
  {"context_delete", (PyCFunction)canvas_context_delete, METH_NOARGS,
   "context_delete()\nDelete the selected object or raster region"},
  {"context_flip_horizontal", (PyCFunction)canvas_context_flip_horizontal, METH_NOARGS,
   "context_flip_horizontal()\nFlips the image or selection horizontally"},
  {"context_flip_vertical", (PyCFunction)canvas_context_flip_vertical, METH_NOARGS,
   "context_flip_vertical()\nFlips the image or selection vertically"},
  {"context_rotate_90CW", (PyCFunction)canvas_context_rotate90CW, METH_NOARGS,
   "context_rotate_90CW\nRotates the image or canvas 90 degrees clock-wise"},
  {"set_grid", (PyCFunction)canvas_set_grid, METH_VARARGS,
   "Set the grid size"},
  {"get_grid", (PyCFunction)canvas_get_grid, METH_NOARGS,
   "Get the grid size"},
  {"grid_on", (PyCFunction)canvas_grid_on, METH_NOARGS,
   "grid_on()->b\nTrue if the grid is enabled"},
  {"set_grid_on", (PyCFunction)canvas_set_grid_on, METH_NOARGS,
   "enable the grid"},
  {"set_grid_off", (PyCFunction)canvas_set_grid_off, METH_NOARGS,
   "disable the grid"},
  {"delete_objects", (PyCFunction)canvas_delete_objects, METH_VARARGS,
   "delete_objects( objects )\nDeletes the objects in the sequence from the image"},
  {"desaturate", (PyCFunction)canvas_desaturate, METH_NOARGS,
   "desaturate()\nDesaturate the image"},
  {"desaturate_weighted", (PyCFunction)canvas_desaturate_weighted, METH_NOARGS,
   "desaturate_weighted()\nDesaturate the image with weighted intensity"},
  {"context_flatten", (PyCFunction)canvas_context_flatten, METH_NOARGS,
   "context_flatten()\nFlattens all objects or all selected objects into the background (rasterize)."},
  {"get_filename", (PyCFunction)canvas_filename, METH_NOARGS,
   "get_filename() -> filename\nReturns the filename, if the image has been saved."},
  {"get_id", (PyCFunction)canvas_get_id, METH_NOARGS,
   "get_id() -> canvas_id\nReturns the id that identifies the canvas in this Faint session"},
  {"get_objects", (PyCFunction)canvas_get_objects, METH_NOARGS, "get_objects() -> [ object1, object2, ... ]\nReturns the objects in the image"},
  {"get_mouse_pos", (PyCFunction)canvas_get_mouse_pos, METH_NOARGS, "get_mouse_pos() -> (x,y)\nReturns the mouse position relative to the image"},
  {"get_selection", (PyCFunction)canvas_get_selection, METH_NOARGS,
   "get_selection() -> (x,y,w,h)\nReturns the selection rectangle or None"},
  {"get_selected", (PyCFunction)canvas_get_selected_objects, METH_NOARGS, "get_sel_obj() -> [ object1, object2, ... ]\nReturns a list of the currently selected objects"},
  {"invert", (PyCFunction)canvas_invert, METH_NOARGS,
   "invert()\nInvert the colors of the image"},
  {"get_size", (PyCFunction)canvas_get_size, METH_NOARGS,
   "get_size() -> (w,h)\nReturns the size of the image."},
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
   "select( object(s) )\nSelects the object or list of objects specified.\nThe previous selection will be discarded"},
  {"set_pixel", (PyCFunction)canvas_set_pixel, METH_VARARGS,
   "set_pixel(x,y,(r,g,b[,a]))\nSets the pixel x,y to the specified color"},
  {"get_pixel", (PyCFunction)canvas_get_pixel, METH_VARARGS,
   "get_pixel(x,y) -> (r,g,b,a)\nReturns the color at x,y (ignoring objects)"},
  {"get_background_png_string", (PyCFunction)canvas_get_background_png_string, METH_NOARGS,
   "get_background_png_string() -> s\nReturns the background image as a png string"},  
  {"set_rect", (PyCFunction)canvas_set_rect, METH_VARARGS,
   "set_rect(x,y,w,h)\nSets the image size to w,h extending from x,y.\nx and y may be negative"},
  {"set_selection", (PyCFunction)canvas_set_selection, METH_VARARGS,
   "set_selection(x,y,w,h)\nSets the selection rectangle"},
  {"shrink_selection", (PyCFunction)canvas_shrink_selection, METH_VARARGS,
   "Shrink the selection"}, // Fixme
  {"set_size", (PyCFunction)canvas_set_size, METH_VARARGS,
   "set_size(w,h)\nSets the image size to w,h"},
  {"undo", (PyCFunction)canvas_undo, METH_NOARGS,
   "undo()\nUndo the last action"},
  {"scroll_page_left", (PyCFunction)canvas_scroll_page_left, METH_NOARGS,
   "Scroll the image one page to the left"},
  {"scroll_page_right", (PyCFunction)canvas_scroll_page_right, METH_NOARGS,
   "Scroll the image one page to the right"},
  {"scroll_page_up", (PyCFunction)canvas_scroll_page_up, METH_NOARGS,
   "Scroll the image one page up"},
  {"scroll_page_down", (PyCFunction)canvas_scroll_page_down, METH_NOARGS,
   "Scroll the image one page down"},
  {"scroll_max_left", (PyCFunction)canvas_scroll_max_left, METH_NOARGS,
   "Scroll the image to the left"},
  {"scroll_max_right", (PyCFunction)canvas_scroll_max_right, METH_NOARGS,
   "Scroll the image to the right"},
  {"scroll_max_up", (PyCFunction)canvas_scroll_max_up, METH_NOARGS,
   "Scroll the image up"},
  {"scroll_max_down", (PyCFunction)canvas_scroll_max_down, METH_NOARGS,
   "Scroll the image to the bottom"},
  {0,0,0,0}  // Sentinel
};

// Python standard methods follow...
static PyObject* canvas_new(PyTypeObject* type, PyObject*, PyObject*){
  canvasObject *self;
  self = (canvasObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* canvas_repr(canvasObject* self ){
  char dbg[1024];
  if ( CanvasOK( self->id ) ){
    sprintf( dbg, "Canvas #%d %s", self->id.Raw(), self->canvas->GetFilename().c_str() );
  }
  else{
    sprintf( dbg, "Retired Canvas #%d", self->id.Raw() );
  }
  return Py_BuildValue("s", dbg);
}

const char* canvasErr = "Incorrect parameters. Canvas supports: Canvas(width, height)\nCanvas(width, height, (r,g,b))\nCanvas(filename)";

std::string TranslatePath( const std::string& path ){
  if ( path.size() == 0 ){
    return path;
  }
  if ( path[0] == '~' ){
    return GetHomeDir() + path.substr( 1, path.size() - 1 );
  }

  return path;
}

// Canvas-constructor, adds a new canvas to faint
static int canvas_init(canvasObject* self, PyObject* args, PyObject* ){
  AppContext& ctx = GetAppContext();

  const int n = PySequence_Length( args );
  if ( n == 0 ){
    self->canvas = &( ctx.NewDocument( ctx.GetDefaultCanvasInfo() ) );
    self->id = self->canvas->GetId();
    return 0;
  }
  if ( n == 1 ){
    char* s_value;
    if ( PyArg_ParseTuple( args, "s", &s_value ) ){
      CanvasInterface* canvas = ctx.Load( TranslatePath( s_value ).c_str() );
      if ( canvas == 0 ){
	PyErr_SetString(PyExc_ValueError, "Failed loading"); // Fixme: Improve error text
	return -1;
      }
      self->canvas = canvas;
      self->id = self->canvas->GetId();
      return 0;
    }
  }
  else if ( n == 2 ){
    int w, h;
    if ( !PyArg_ParseTuple( args, "ii", &w, &h ) ){
      PyErr_SetString( PyExc_ValueError, canvasErr );
      return -1;
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString( PyExc_ValueError, "Negative width or height passed to Canvas constructor");
      return -1;
    }
    self->canvas = &( ctx.NewDocument(w, h) );
    self->id = self->canvas->GetId();
    return 0;
  }

  else if ( n == 3 ){
    int w, h, r, g, b;
    if ( !PyArg_ParseTuple( args, "ii(iii)", &w, &h, &r, &g, &b ) ){
      PyErr_SetString( PyExc_ValueError, canvasErr );
      return -1;
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString( PyExc_ValueError, "Negative width or height passed to Canvas constructor");
      return -1;
    }
    CanvasInfo info = ctx.GetDefaultCanvasInfo();
    info.width = w;
    info.height = h;
    info.backgroundColor = faint::Color( r, g, b );
    self->canvas = &( ctx.NewDocument( info ) );
    self->id = self->canvas->GetId();
    return 0;
  }
  PyErr_SetString( PyExc_ValueError, canvasErr );
  return -1;
}

PyTypeObject CanvasType = {
  PyObject_HEAD_INIT(NULL)
  0, // ob_size
  "Canvas", //tp_name
  sizeof(canvasObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)canvas_repr, // tp_repr
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
  "A Faint document (i.e. an image open in a Faint tab)", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  canvas_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
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

PyObject* Pythoned( CanvasInterface& canvas ){
  canvasObject* py_canvas = (canvasObject*)CanvasType.tp_alloc( &CanvasType, 0 );
  py_canvas->canvas = &canvas;
  py_canvas->id = py_canvas->canvas->GetId();
  return (PyObject*) py_canvas;
}
