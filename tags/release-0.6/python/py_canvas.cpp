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
#include "commands/addframecommand.hh"
#include "commands/cmdgroupobjects.hh"
#include "commands/cmdsetbitmap.hh"
#include "commands/drawobjectcommand.hh"
#include "commands/resizecommand.hh"
#include "commands/set-frame-delay.hh"
#include "geo/grid.hh"
#include "objects/objellipse.hh"
#include "objects/objline.hh"
#include "objects/objraster.hh"
#include "objects/objrectangle.hh"
#include "objects/objtext.hh"
#include "objects/objtri.hh"
#include "python/pythoninclude.hh"
#include "python/py_canvas.hh"
#include "python/py_create_object.hh"
#include "python/py_grid.hh"
#include "python/py_util.hh"
#include "python/pyinterface.hh"
#include "rendering/cairocontext.hh"
#include "util/autocrop.hh"
#include "util/clipboard.hh"
#include "util/commandutil.hh"
#include "util/cursorposinfo.hh"
#include "util/image.hh"
#include "util/imageutil.hh"
#include "util/objutil.hh"
#include "util/pathutil.hh"
#include "util/save.hh"
#include "util/settings.hh"
#include "util/settingutil.hh"
#include "util/util.hh"
#include "util/toolutil.hh"

void grid_off( canvasObject* self ){
  Grid g( self->canvas->GetGrid() );
  g.SetEnabled( false );
  g.SetVisible( false );
  self->canvas->SetGrid( g );
  faint::python_queue_refresh(self->canvas);
}

PyObject* canvas_removed_error();

template<Object* (*Creator)(PyObject*)>
static PyObject* canvas_add_object(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Object* obj = Creator( args );
  if ( obj == 0 ){
    return 0;
  }
  faint::python_run_command(self->canvas, get_add_object_command(obj, select_added(false)));
  return pythoned(obj, self->canvas);
}

static PyObject* canvas_auto_crop(canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Command* cmd = get_auto_crop_command(self->canvas->GetImage());
  if ( cmd != 0 ){
    faint::python_run_command( self->canvas, cmd );
  }
  return Py_BuildValue("");
}

PyObject* canvas_center_view( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Point out;
  if ( !parse_point(args, &out) ){
    return 0;
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
    return 0;
  }
  const faint::Bitmap& srcBmp(self->canvas->GetBitmap());
  if ( !faint::inside(rect, srcBmp) ){
    PyErr_SetString(PyExc_ValueError, "Rectangle not fully inside image.");
    return 0;
  }
  faint::Bitmap bmp(cairo_compatible_sub_bitmap(srcBmp, rect));
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    PyErr_SetString(PyExc_OSError, "Failed opening clipboard.");
    return 0;
  }
  clipboard.SetBitmap(bmp, GetAppContext().GetToolSettings().Get(ts_BgCol));
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
  self->canvas->ContextCrop();
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
  self->canvas->ContextFlatten();
  return Py_BuildValue("");
}

static PyObject* canvas_context_flip_horizontal(canvasObject* self, PyObject* ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ContextFlipHorizontal();
  return Py_BuildValue("");
}

static PyObject* canvas_context_flip_vertical(canvasObject* self, PyObject* ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ContextFlipVertical();
  return Py_BuildValue("");
}

static PyObject* canvas_context_rotate90CW(canvasObject* self, PyObject* ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->ContextRotate90CW();
  return Py_BuildValue("");
}

static PyObject* canvas_delete_objects( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  objects_t objects;
  if ( !objects_from_args( args, objects ) ){
    return 0;
  }
  CanvasInterface* canvas = self->canvas;
  for ( size_t i = 0; i != objects.size(); i++ ){
    if ( !canvas->Has( objects[i]->GetId() ) ){
      PyErr_SetString(PyExc_ValueError, "Atleast one object is not in this canvas.");
      return 0;
    }
  }

  faint::python_run_command( canvas,
    get_delete_objects_command( objects, canvas->GetImage() ) );
  return Py_BuildValue("");
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
  if ( n == 1 ){
    PyObject* first = PySequence_GetItem( sequence, 0 );
    if ( PySequence_Check( first ) ){
      // Unwrapped the single sequence argument (allows deselect([o1,o2]))
      sequence = first;
      n = PySequence_Length( sequence );
    }
  }

  std::vector<smthObject*> objects;
  for ( int i = 0; i != n; i++ ){
    PyObject* item = PySequence_GetItem( sequence, i );
    smthObject* obj = as_smthObject( item );
    if ( obj == 0 ){
      PyErr_SetString(PyExc_TypeError, "That's not a Faint object.");
      return 0;
    }
    if ( !self->canvas->Has( obj->objectId ) ){
      PyErr_SetString(PyExc_ValueError, "The canvas does not contain that item.");
      return 0;
    }
    objects.push_back(obj);
  }

  for ( size_t i = 0; i != objects.size(); ++i ){
    self->canvas->DeselectObject(objects[i]->obj);
  }
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
    return 0;
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
  faint::Color eraser;
  if ( PySequence_Length(args) == 1 ){
    bool ok = parse_color( PySequence_GetItem( args, 0 ), keep );
    if ( !ok ){
      return 0;
    }
    eraser = GetAppContext().GetToolSettings().Get( ts_BgCol );
  }
  else if ( PySequence_Length(args) == 2 ){
    bool colorsOK = parse_color( PySequence_GetItem( args, 0 ), keep ) &&
      parse_color( PySequence_GetItem( args, 1 ), eraser );
    if ( !colorsOK ){
      return 0;
    }
  }
  if ( keep == eraser ){
    PyErr_SetString(PyExc_ValueError, "Same erase color as the kept color");
    return 0;
  }
  faint::python_run_command(self->canvas, get_erase_but_color_command(keep, eraser));
  return Py_BuildValue("");
}

static PyObject* canvas_filename( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  std::string filename = self->canvas->GetFilename();
  if ( filename.size() == 0 ){
    return Py_BuildValue("");
  }
  return Py_BuildValue( "s", filename.c_str() );
}

static PyObject* canvas_flatten( canvasObject* self, PyObject* args ){
  if (!canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  objects_t objects;
  if ( !parse_objects(args, &objects) ){
    return 0;
  }

  const faint::Image& image = self->canvas->GetImage();
  if ( !faint::has_all(image, objects) ){
    PyErr_SetString(PyExc_ValueError, "Objects not in this canvas");
    return 0;
  }
  faint::python_run_command(self->canvas, get_flatten_command(objects, image));
  return Py_BuildValue("");
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
    return 0;
  }
  faint::python_run_command(self->canvas, new AddFrameCommand(size));
  return Py_BuildValue("");
}

static PyObject* canvas_frame_get_delay( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }
  int num;
  if ( !PyArg_ParseTuple(args, "i", &num ) ){
    return 0;
  }
  if ( num < 0 || self->canvas->GetNumFrames() <= num ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index");
    return 0;
  }
  int delay = self->canvas->GetFrameDelay(num);
  return Py_BuildValue("i", delay);
}

static PyObject* canvas_frame_next( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->NextFrame(); // Fixme
  return Py_BuildValue("");
}

static PyObject* canvas_frame_previous( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id ) ){
    return canvas_removed_error();
  }
  self->canvas->PreviousFrame(); // Fixme
  return Py_BuildValue("");
}

static PyObject* canvas_frame_set_delay( canvasObject* self, PyObject* args ){
  if ( !canvas_ok(self->id) ){
    return canvas_removed_error();
  }
  int num, delay;
  if ( !PyArg_ParseTuple(args, "ii", &num, &delay ) ){
    return 0;
  }
  if ( delay < 0 ){ // Fixme: -1 should probably mean 'forever' for gifs.
    PyErr_SetString(PyExc_ValueError, "Negative delay");
    return 0;
  }
  if ( num < 0 || self->canvas->GetNumFrames() <= num ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index");
    return 0;
  }
  
  delay_t oldDelay(self->canvas->GetFrameDelay(num));
  delay_t newDelay(delay);
  self->canvas->RunCommand(set_frame_delay_command(num, New(newDelay), Old(oldDelay)));
  return Py_BuildValue("");
}

static PyObject* to_py_png_string( const faint::Bitmap& bmp ){
  std::string pngString( to_png_string( bmp ) );
  PyObject* pythonString = PyString_FromStringAndSize( pngString.c_str(), pngString.size() );
  return pythonString;
}

static PyObject* canvas_get_background_png_string( canvasObject* self, PyObject* args, PyObject* kwArgs ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  char* keywordList[] = {"stamp", 0};
  int in_stamp = -1;
  PyArg_ParseTupleAndKeywords(args, kwArgs, "|i", keywordList, &in_stamp); // Fixme: Replace with p for Python 3
  if ( in_stamp == -1 ){
    PyErr_SetString(PyExc_ValueError, "Keyword 'stamp' missing.");
    return 0;
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

static PyObject* canvas_get_grid_f(canvasObject* self){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Grid g = self->canvas->GetGrid();
  int size = g.Spacing();
  return Py_BuildValue("i", size);
}

static PyObject* canvas_get_id( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  return Py_BuildValue( "i", self->id.Raw() );
}

static PyObject* canvas_get_mouse_pos( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  IntPoint p( truncated(self->canvas->GetRelativeMousePos()) );
  return Py_BuildValue("ii", p.x, p.y);
}

static PyObject* canvas_get_objects( canvasObject* self ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  const objects_t& objects = self->canvas->GetObjects();
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i!= objects.size(); i++ ){
    PyList_Append( py_list, pythoned( objects[i], self->canvas ) );
  }
  return py_list;
}

static PyObject* canvas_get_pixel( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  IntPoint pos;
  if ( !parse_intpoint(args, &pos) ){
    return 0;
  }
  const faint::Bitmap& bmp = self->canvas->GetImage().GetBitmap();
  if ( invalid_pixel_pos(pos, bmp) ){
    return 0;
  }
  // To include objects, the CursorPositionInfo must be retrieved.
  // (As an alternative, one could draw the background and all objects
  // for the pixel, and then fetch it but this would give blended
  // colors for edges of objects, as well as the blended color for
  // alpha-objects, which may be undesired)
  CursorPositionInfo posInfo(self->canvas->GetPosInfo(pos));
  faint::Color color = get_hovered_color(posInfo);
  return build_color_tuple(color);
}

PyObject* canvas_get_selected_objects( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  const objects_t& objects = self->canvas->GetObjectSelection();
  PyObject* py_list = PyList_New(0);
  for ( size_t i = 0; i!= objects.size(); i++ ){
    PyObject* smth = pythoned(objects[i], self->canvas);
    PyList_Append( py_list, smth );
  }
  return py_list;
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

static PyObject* canvas_grid_on(canvasObject* self){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  Grid g(self->canvas->GetGrid() );
  return python_bool(g.Enabled());
}

PyObject* canvas_invert( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::python_run_command( self->canvas, get_invert_command() );
  return Py_BuildValue("");
}

static PyObject* canvas_line(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Point p0, p1;
  if ( !parse_point2( args, &p0, &p1) ){
    PyErr_SetString( PyExc_TypeError, "Invalid parameters to line( x0, y0, x1, y1 )" );
    return 0;
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
    PyErr_SetString( PyExc_TypeError, "A group must contain at least one object." );
    return 0;
  }

  objects_t faintObjects;
  for ( int i = 0; i != n; i++ ){
    PyObject* object = PySequence_GetItem( sequence, i );
    if ( !PyObject_IsInstance( object, (PyObject*)&SmthType ) ){
      PyErr_SetString( PyExc_TypeError, "Unsupported item in list" );
      return 0;
    }
    faintObjects.push_back( ((smthObject*)object)->obj );
  }

  GroupObjects* cmd = new GroupObjects( faintObjects, select_added(false) );
  faint::python_run_command( self->canvas, cmd );
  return pythoned( cmd->GetComposite(), self->canvas );
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
    return 0;
  }

  Settings settings( default_line_settings() );
  settings.Update( GetAppContext().GetToolSettings() );
  ObjTri* objTri = new ObjTri( tri, settings );
  faint::python_run_command( self->canvas, get_add_object_command(objTri, select_added(false)) );
  return pythoned( objTri, self->canvas );;
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
    return 0;
  }
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    PyErr_SetString(PyExc_OSError, "Failed opening clipboard.");
    return 0;
  }

  faint::Bitmap bmp;
  if ( !clipboard.GetBitmap(bmp) ){
    PyErr_SetString(PyExc_OSError, "No bitmap in clipboard.");
    return 0;
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
    return 0;
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

PyObject* canvas_removed_error(){
  PyErr_SetString( PyExc_ValueError, "Operation attempted on closed canvas." );
  return 0;
}

PyObject* canvas_replace_alpha( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  faint::Color bgColor;
  if ( !parse_rgb_color( args, bgColor ) ){
    return 0;
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
    return 0;
  }

  faint::python_run_command( self->canvas,
    get_replace_color_command(Old(color1), New(color2)));
  return Py_BuildValue("");
}

static PyObject* canvas_save(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
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
      PyErr_SetString( PyExc_ValueError, "Failed saving" ); // Fixme
    }
  }

  // Use the filename passed as argument
  char* filename = 0;
  if ( !PyArg_ParseTuple( args, "s", &filename ) ){
    return 0;
  }
  if ( !valid_save_filename( filename ) ){
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

static PyObject* canvas_save_backup( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  if ( PySequence_Length( args ) == 0 ){
    PyErr_SetString( PyExc_ValueError, "No backup filename specified" );
    return 0;
  }

  char* filename = 0;
  if ( PyArg_ParseTuple( args, "s", &filename ) ){
    if ( !valid_save_filename( filename ) ){
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

static PyObject* canvas_select_object( canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
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
      // Unwrapped the single sequence argument (allows select([o1,o2]))
      sequence = first;
      n = PySequence_Length( sequence );
    }
  }

  std::vector<smthObject*> objects;
  for ( int i = 0; i != n; i++ ){
    PyObject* item = PySequence_GetItem( sequence, i );
    smthObject* obj = as_smthObject( item );
    if ( obj == 0 ){
      PyErr_SetString(PyExc_TypeError, "That's not a Faint object.");
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
    self->canvas->SelectObject( objects[i]->obj, deselect_old(false) );
  }
  return Py_BuildValue("");
}

int canvas_set_grid( canvasObject* self, PyObject* args, void* ){
  return -1; // Fixme
}

static PyObject* canvas_set_grid_f(canvasObject* self, PyObject* args ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
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

  self->canvas->SetGrid( Grid(true, true, size, faint::Color(255,0,255) ) );
  faint::python_queue_refresh(self->canvas);
  return Py_BuildValue( "" );
}

static PyObject* canvas_set_grid_off(canvasObject* self){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  grid_off( self );
  return Py_BuildValue("");
}

static PyObject* canvas_set_grid_on(canvasObject* self){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  Grid g( self->canvas->GetGrid() );
  g.SetEnabled( true );
  g.SetVisible( true );
  self->canvas->SetGrid(g );
  faint::python_queue_refresh(self->canvas);
  return Py_BuildValue( "" );
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

  const faint::Bitmap& bmp(self->canvas->GetBitmap());
  if ( invalid_pixel_pos(x, y, bmp) ){
    return 0;
  }

  // Fixme: Need a command. Not one per pixel though...
  // faint::put_pixel_raw( bmp, x,y, faint::color_from_ints( r, g, b, a ) );
  return Py_BuildValue("");
}

static PyObject* canvas_set_point_overlay(canvasObject* self, PyObject* args){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }
  IntPoint p;
  if ( !parse_intpoint(args, &p) ){
    return 0;
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
      return 0;
    }
  }

  faint::Color bgCol = GetAppContext().GetToolSettings().Get( ts_BgCol );
  Command* resize = new ResizeCommand( IntRect( IntPoint(x, y), IntSize(w, h) ), bgCol  );
  faint::python_run_command(self->canvas, resize);
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
  faint::python_queue_refresh(self->canvas);
  return Py_BuildValue("");
}

PyObject* canvas_shrink_selection( canvasObject* self, PyObject* ){
  if ( !canvas_ok( self->id ) ){
    return canvas_removed_error();
  }

  IntRect selection(self->canvas->GetRasterSelection().GetRect());
  if ( empty( selection ) ){
    IntRect cropRect;
    if ( get_auto_crop_rect( self->canvas->GetBitmap(), cropRect ) ){
      self->canvas->SetRasterSelection( translated( cropRect, selection.TopLeft() ) );
    }
  }
  else {
    faint::Bitmap bmpSelected( cairo_compatible_sub_bitmap( self->canvas->GetBitmap(), selection ) );
    IntRect cropRect;
    if ( get_auto_crop_rect( bmpSelected, cropRect ) ){
      self->canvas->SetRasterSelection( translated( cropRect, selection.TopLeft() ) );
    }
  }
  faint::python_queue_refresh(self->canvas);
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
      return 0;
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString(PyExc_ValueError, "Negative or zero width or height argument");
      return 0;
    }
    faint::Color bgCol = GetAppContext().GetToolSettings().Get( ts_BgCol );
    faint::python_run_command( self->canvas, new ResizeCommand( rect_from_size(IntSize(w, h)), bgCol ) );
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

  faint::python_run_command( self->canvas,
    new ResizeCommand( rect_from_size(IntSize(w, h)), faint::color_from_ints(r,g,b,a) ));
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
   (char*) "Canvas grid", NULL},
  {0,0,0,0,0} // Sentinel
};

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
   "auto_crop()\n Autocrops the image."},
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
  {"context_flip_vertical", (PyCFunction)canvas_context_flip_vertical, METH_NOARGS,
   "context_flip_vertical()\nFlips the image or selection vertically"},
  {"context_rotate_90CW", (PyCFunction)canvas_context_rotate90CW, METH_NOARGS,
   "context_rotate_90CW()\nRotates the image or canvas 90 degrees clock-wise"},
  {"clear_point_overlay", (PyCFunction)canvas_clear_point_overlay, METH_NOARGS,
   "clear_point_overlay()\n->Clear indicated point (see set_point_overlay)."},
  {"copy_rect", (PyCFunction)canvas_copy_rect, METH_VARARGS,
   "copy_rect(x,y,w,h)\nCopies the specified rectangle to the clipboard"},
  {"erase_but_color", (PyCFunction)canvas_erase_but_color, METH_VARARGS,
   "erase_but_color(keptColor[,eraseColor])\nReplaces all colors, except keptColor, with eraseColor. Uses the current secondary color if keptColor is omitted."},
  {"set_grid", (PyCFunction)canvas_set_grid_f, METH_VARARGS,
   "Set the grid size"}, // Fixme: doc str format
  {"get_frame_delay", (PyCFunction)canvas_frame_get_delay, METH_VARARGS,
   "get_frame_delay(index) -> delay\nReturns the delay for the specified frame."},
  {"get_grid", (PyCFunction)canvas_get_grid_f, METH_NOARGS,
   "Get the grid size"}, // Fixme: doc_str format
  {"grid_on", (PyCFunction)canvas_grid_on, METH_NOARGS,
   "grid_on()->b\nTrue if the grid is enabled"},
  {"paste", (PyCFunction)canvas_paste, METH_VARARGS,
   "paste(x,y)\nPastes a bitmap from the clipboard to (x,y)"},
  {"set_grid_on", (PyCFunction)canvas_set_grid_on, METH_NOARGS,
   "set_grid_on()\nenable the grid"},
  {"set_grid_off", (PyCFunction)canvas_set_grid_off, METH_NOARGS,
   "set_grid_off()\ndisable the grid"},
  {"set_point_overlay", (PyCFunction)canvas_set_point_overlay, METH_VARARGS,
   "set_point_overlay(x,y)\n->Indicate a point."},
  {"delete_objects", (PyCFunction)canvas_delete_objects, METH_VARARGS,
   "delete_objects(objects)\nDeletes the objects in the sequence from the image"},
  {"desaturate", (PyCFunction)canvas_desaturate, METH_NOARGS,
   "desaturate()\nDesaturate the image"},
  {"desaturate_weighted", (PyCFunction)canvas_desaturate_weighted, METH_NOARGS,
   "desaturate_weighted()\nDesaturate the image with weighted intensity"},
  {"context_flatten", (PyCFunction)canvas_context_flatten, METH_NOARGS,
   "context_flatten()\nFlattens all objects or all selected objects into the background (rasterize)."},
  {"flatten", (PyCFunction)canvas_flatten, METH_VARARGS,
   "flatten(object1[, object2, ...])\nFlattens the specified objects."},
  {"get_filename", (PyCFunction)canvas_filename, METH_NOARGS,
   "get_filename() -> filename\nReturns the filename, if the image has been saved."},
  {"get_id", (PyCFunction)canvas_get_id, METH_NOARGS,
   "get_id() -> canvas_id\nReturns the id that identifies the canvas in this Faint session"},
  {"get_objects", (PyCFunction)canvas_get_objects, METH_NOARGS, "get_objects() -> [ object1, object2, ... ]\nReturns the objects in the image"},
  {"get_mouse_pos", (PyCFunction)canvas_get_mouse_pos, METH_NOARGS,
   "get_mouse_pos() -> (x,y)\nReturns the mouse position relative to the image"},
  {"get_selection", (PyCFunction)canvas_get_selection, METH_NOARGS,
   "get_selection() -> (x,y,w,h)\nReturns the selection rectangle or None"},
  {"get_selected", (PyCFunction)canvas_get_selected_objects, METH_NOARGS, "get_sel_obj() -> [ object1, object2, ... ]\nReturns a list of the currently selected objects"},
  {"get_zoom", (PyCFunction)canvas_get_zoom, METH_NOARGS,
   "get_zoom() -> zoom\nReturns the current zoom as a floating point value"},
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
  {"set_pixel", (PyCFunction)canvas_set_pixel, METH_VARARGS,
   "set_pixel(x,y,(r,g,b[,a]))\nSets the pixel x,y to the specified color"},
  {"get_pixel", (PyCFunction)canvas_get_pixel, METH_VARARGS,
   "get_pixel(x,y) -> (r,g,b,a)\nReturns the color at x,y"},

  // Casting function with keywords to PyCFunction causes warnings, as method signature doesn't match.
  // Hopefully benign.
  {"get_background_png_string", (PyCFunction)canvas_get_background_png_string, METH_VARARGS|METH_KEYWORDS,
   "get_background_png_string(stamp=True|False) -> s\nReturns the background encoded in png as a string.\nIf the keyword argument stamp is True, any floating raster selection will be stamped onto the image."},
  {"set_frame_delay", (PyCFunction)canvas_frame_set_delay, METH_VARARGS,
   "set_frame_delay( frame, ms )\nSets the delay in milliseconds for the specified frame"},
  {"set_rect", (PyCFunction)canvas_set_rect, METH_VARARGS,
   "set_rect(x,y,w,h)\nSets the image size to w,h extending from x,y.\nx and y may be negative"},
  {"set_selection", (PyCFunction)canvas_set_selection, METH_VARARGS,
   "set_selection(x,y,w,h)\nSets the selection rectangle"},
  {"shrink_selection", (PyCFunction)canvas_shrink_selection, METH_VARARGS,
   "shrink_selection()\nAuto-shrink the selection rectangle to an image detail by trimming same-colored areas from its sides"},
  {"set_size", (PyCFunction)canvas_set_size, METH_VARARGS,
   "set_size(w,h)\nSets the image size to w,h"},
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
    std::string filename = self->canvas->GetFilename();
    if ( !filename.empty() ){
      ss << " " << filename;
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
    // Return -1 (self is less) for type mismatch
    return -1;
  }
  canvasObject* other((canvasObject*)otherRaw);
  CanvasId id(self->canvas->GetId());
  CanvasId otherId(other->canvas->GetId());
  if ( id < otherId ){
    return -1;
  }
  else if ( id == otherId ){
    return 0;
  }
  return 1;
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

const char* canvasErr = "Incorrect parameters. Canvas supports: Canvas(width, height)\nCanvas(width, height, (r,g,b[,a]))\nCanvas(filename(s))";



// Canvas-constructor, adds a new canvas to faint
static int canvas_init(canvasObject* self, PyObject* args, PyObject* ){
  AppContext& ctx = GetAppContext();

  const int n = PySequence_Length( args );
  if ( faint::is_string_sequence(args) ){
    std::vector<std::string> paths = faint::parse_string_sequence(args);
    CanvasInterface* canvas = ctx.LoadAsFrames(paths, change_tab(true));
    if ( canvas == 0 ){
      PyErr_SetString(PyExc_ValueError, "Failed loading");
      return -1;
    }
    self->canvas = canvas;
    self->id = self->canvas->GetId();
    return 0;
  }
  else if ( n == 0 ){
    // No constructor arguments create a default canvas.
    self->canvas = &( ctx.NewDocument( ctx.GetDefaultImageInfo() ) );
    self->id = self->canvas->GetId();
    return 0;
  }
  else if ( n == 1 ){
    char* s_value;
    if ( PyArg_ParseTuple( args, "s", &s_value ) ){
      CanvasInterface* canvas = ctx.Load( TranslatePath( s_value ).c_str(), change_tab(true) );
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
      PyErr_SetString( PyExc_TypeError, canvasErr );
      return -1;
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString( PyExc_ValueError, "Zero or negative Width or height passed to Canvas constructor");
      return -1;
    }
    self->canvas = &( ctx.NewDocument(ImageInfo(IntSize(w, h))));
    self->id = self->canvas->GetId();
    return 0;
  }

  else if ( n == 3 ){
    int w, h, r, g, b;
    int a = 255;
    bool rgb = PyArg_ParseTuple( args, "ii(iii)", &w, &h, &r, &g, &b ) != 0;
    if ( !rgb ){
      PyErr_Clear();
      bool rgba = PyArg_ParseTuple( args, "ii(iiii)", &w, &h, &r, &g, &b, &a ) != 0;
      if ( !rgba ){
	PyErr_SetString( PyExc_TypeError, canvasErr );
	return -1;
      }
    }
    if ( w <= 0 || h <= 0 ){
      PyErr_SetString( PyExc_ValueError, "Negative width or height passed to Canvas constructor");
      return -1;
    }

    if ( invalid_color(r,g,b,a) ){
      return -1;
    }

    self->canvas = &( ctx.NewDocument( ImageInfo(IntSize(w,h), faint::color_from_ints(r,g,b,a) ) ) );
    self->id = self->canvas->GetId();
    return 0;
  }
  PyErr_SetString( PyExc_TypeError, canvasErr );
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
