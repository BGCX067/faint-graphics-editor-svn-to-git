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

#include <vector>
#include <string>
#include "app/get-app-context.hh"
#include "bitmap/bitmap.hh"
#include "commands/change-setting-cmd.hh"
#include "commands/obj-function-cmd.hh"
#include "commands/text-entry-cmd.hh"
#include "commands/tri-cmd.hh"
#include "geo/intrect.hh"
#include "geo/rect.hh"
#include "objects/objcomposite.hh"
#include "objects/objellipse.hh"
#include "objects/objline.hh"
#include "objects/objpath.hh"
#include "objects/objpolygon.hh"
#include "objects/objraster.hh"
#include "objects/objrectangle.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "python/py-include.hh"
#include "python/py-canvas.hh"
#include "python/py-settings.hh"
#include "python/py-something.hh"
#include "python/py-tri.hh"
#include "python/py-ugly-forward.hh"
#include "python/py-interface.hh"
#include "util/serialize-bitmap.hh"
#include "util/setting-util.hh"
#include "util/canvas.hh"
#include "util/command-util.hh"
#include "util/font.hh"
#include "util/image.hh"
#include "util/object-util.hh"

namespace faint{

bool faint_side_ok(smthObject* self){
  if (canvas_ok(self->canvasId) &&
    self->canvas->Has(self->frameId) &&
    self->canvas->GetFrameById(self->frameId).Has(self->objectId)){
    return true;
  }
  return false;
}

void show_error(smthObject*){
  PyErr_SetString(PyExc_ValueError, "That object is removed.");
}

inline const Image& get_image(smthObject* self){
  return self->canvas->GetFrameById(self->frameId);
}

PyObject* pythoned(Object* faintObj, Canvas* canvas, const FrameId& frameId){
  smthObject* smth = (smthObject*)SmthType.tp_alloc(&SmthType, 0);
  smth->obj = faintObj;
  smth->objectId = faintObj->GetId();
  smth->canvas = canvas;
  smth->canvasId = canvas->GetId();
  smth->frameId = frameId;
  return (PyObject*)smth;
}

static void Smth_update_settings(const BoundObject& self, const Settings& s){
  python_run_command(self, change_settings_command(self.obj, New(s),
      Old(self.obj->GetSettings())));
}

static void Smth_moveto(const BoundObject& self, const Point& pos){
  Tri oldTri = self.obj->GetTri();
  Point diff = pos - oldTri.P0();
  Tri newTri = translated(oldTri, diff.x, diff.y);

  python_run_command(self, new TriCommand(self.obj,
    New(newTri), Old(oldTri)));
}

static void Smth_rotate(const BoundObject& self, const radian& angle, const Optional<Point>& origin){
  Tri oldTri(self.obj->GetTri());
  python_run_command(self,
    get_rotate_command(self.obj, Angle::Rad(angle),
      origin.Or(center_point(oldTri))));
}

static void Smth_skew(const BoundObject& self, const coord& skew){
  Object* obj = self.obj;
  Tri oldTri(obj->GetTri());
  Tri newTri(skewed(oldTri, skew));

  python_run_command(self,
    new TriCommand(obj, New(newTri), Old(oldTri)));
}

static Angle Smth_get_angle(const BoundObject& self){
  Tri t = self.obj->GetTri();
  return t.GetAngle();
}

static coord Smth_get_skew(const BoundObject& self){
  Tri t = self.obj->GetTri();
  return t.Skew();
}

static Point Smth_pos(const BoundObject& self){
  return self.obj->GetTri().P0();
}

static void Smth_set_linewidth(const BoundObject& self, const coord& lw){
  if (!self.obj->GetSettings().Has(ts_LineWidth)){
    throw ValueError("That object does not support line width.");
  }
  python_run_command(self,
    change_setting_command(self.obj, ts_LineWidth, lw));
}

static coord Smth_get_linewidth(const BoundObject& self){
  const Settings& settings = self.obj->GetSettings();
  if (!settings.Has(ts_LineWidth)){
    throw ValueError("That object does not support line width.");
  }
  return settings.Get(ts_LineWidth);
}

static Settings Smth_get_settings(const BoundObject& self){
  return self.obj->GetSettings();
}

static void Smth_set_linestyle(const BoundObject& self, const utf8_string& s){
  if (!self.obj->GetSettings().Has(ts_LineStyle)){
    throw ValueError("This object does not support line style");
  }
  LineStyle style = LineStyle::SOLID;
  if (s == "solid" || s == "s"){
    style = LineStyle::SOLID;
  }
  else if (s == "long_dash" || s == "ld"){
    style = LineStyle::LONG_DASH;
  }
  else {
    throw ValueError("Invalid string. Valid options are \"solid\" (or \"s\") and \"long_dash\" (or \"ld\")");
  }

  python_run_command(self,
    change_setting_command(self.obj, ts_LineStyle, style));
}

static void Smth_set_angle(const BoundObject& self, const radian& angle){
  Object* obj = self.obj;
  Tri oldTri = obj->GetTri();
  Tri newTri = rotated(oldTri, Angle::Rad(angle) - oldTri.GetAngle(), center_point(oldTri));

  python_run_command(self, new TriCommand(obj, New(newTri), Old(oldTri)));
}

static void Smth_set_angles(const BoundObject& self, const AngleSpan& span){
  if (!is_ellipse(self.obj)){
    throw ValueError("Only supported for ellipses.");
  }
  typedef ObjFunctionCommand<Object, AngleSpan, set_angle_span> SetSpanCmd;
  python_run_command(self,
    new SetSpanCmd(self.obj, "Change Arc Span",
      New(span), Old(get_angle_span(self.obj).Get())));
}

static void Smth_set_fg(const BoundObject& self, const Paint& fg){
  python_run_command(self, change_setting_command(self.obj, ts_Fg, fg));
}

static void Smth_set_bg(const BoundObject& self, const Paint& bg){
  python_run_command(self, change_setting_command(self.obj, ts_Bg, bg));
}

static Paint Smth_get_bg(const BoundObject& self){
  return self.obj->GetSettings().Get(ts_Bg);
}

static Paint Smth_get_fg(const BoundObject& self){
  return self.obj->GetSettings().Get(ts_Fg);
}

static utf8_string Smth_get_linestyle(const BoundObject& self){
  const Settings& settings = self.obj->GetSettings();

  if (!settings.Has(ts_LineStyle)){
    throw ValueError("This object does not have a linestyle attribute.");
  }

  // Fixme: Generate
  LineStyle style = settings.Get(ts_LineStyle);
  if (style == LineStyle::SOLID){
    return "solid";
  }
  else if (style == LineStyle::LONG_DASH){
    return "long_dash";
  }
  else {
    assert(false);
    return "";
  }
}

static Rect Smth_rect(const BoundObject& self){
  return bounding_rect(self.obj->GetTri());
}

static void Smth_set_text(const BoundObject& self, const utf8_string& str){
  ObjText* text = dynamic_cast<ObjText*>(self.obj);
  if (!text){
    throw ValueError(space_sep(self.obj->GetType(), "does not support text."));
  }

  python_run_command(self,
    text_entry_command(text, New(str), Old(text->GetString())));
}

static utf8_string Smth_get_text(const BoundObject& self){
  ObjText* text = dynamic_cast<ObjText*>(self.obj);
  if (!text){
    throw ValueError(space_sep(self.obj->GetType(), "does not support text."));
  }
  return text->GetTextBuffer()->get();
}

static std::string Smth_get_png_string(const BoundObject& self){
  ObjRaster* raster = dynamic_cast<ObjRaster*>(self.obj);
  if (!raster){
    throw ValueError(space_sep(self.obj->GetType(),
      "does not support images."));
  }

  std::string png(to_png_string(raster->GetBitmap()));
  return to_png_string(raster->GetBitmap());
}

static text_lines_t Smth_get_text_lines(const BoundObject& self){
  ObjText* text = dynamic_cast<ObjText*>(self.obj);
  if (!text){
    throw ValueError(space_sep(self.obj->GetType(),
      "does not support text."));
  }

  return split_text_buffer(*(text->GetTextBuffer()), text->GetSettings(), max_width_t(text->GetTri().Width()));
}

static coord Smth_get_text_baseline(const BoundObject& self){
  ObjText* txt = dynamic_cast<ObjText*>(self.obj);
  if (txt == nullptr){
    throw ValueError("This object does not have text attributes");
  }
  return txt->BaselineOffset();
}

static coord Smth_get_text_height(const BoundObject& self){
  ObjText* txt = dynamic_cast<ObjText*>(self.obj);
  if (txt == nullptr){
    throw ValueError("This object does not have text attributes");
  }
  return txt->RowHeight();
}

static utf8_string Smth_get_fillstyle(const BoundObject& self){
  const Settings& settings = self.obj->GetSettings();
  if (!settings.Has(ts_FillStyle)){
    throw ValueError("That object does not support fill styles");
  }

  const FillStyle fillStyle = settings.Get(ts_FillStyle);
  if (fillStyle == FillStyle::BORDER){
    return "border";
  }
  else if (fillStyle == FillStyle::BORDER_AND_FILL){
    return "border+fill";
  }
  else if (fillStyle == FillStyle::FILL){
    return "fill";
  }
  else {
    assert(false);
    return "";
  }
}

static utf8_string Smth_get_transparency(const BoundObject& self){
  // Fixme: These must be generated!
  const Settings& settings = self.obj->GetSettings();
  if (!settings.Has(ts_BackgroundStyle)){
    throw ValueError("That object does not support a background style.");
  }
  const BackgroundStyle bgStyle = settings.Get(ts_BackgroundStyle);
  if (bgStyle == BackgroundStyle::MASKED){
    return "masked";
  }
  return "opaque";
}

static int Smth_get_arrowhead(const BoundObject& self){
  // Fixme: Generate, use strings and so on (Couldn't just remove
  // this, as it is used in write_svg).

  const Settings& settings = self.obj->GetSettings();
  if (!settings.Has(ts_LineArrowhead)){
    throw ValueError("That object does not support arrowheads.");
  }
  return to_int(settings.Get(ts_LineArrowhead));
}

static void Smth_set_fillstyle(const BoundObject& self, const utf8_string& s){
  if (! self.obj->GetSettings().Has(ts_FillStyle)){
    throw ValueError("That object does not support fill styles");
  }

  Command* cmd = nullptr;
  if (s == "fill" || s == "f"){
    cmd = change_setting_command(self.obj, ts_FillStyle, FillStyle::FILL);
  }
  else if (s == "border" || s == "b"){
    cmd = change_setting_command(self.obj, ts_FillStyle, FillStyle::BORDER);
  }
  else if (s == "border+fill" || s == "bf" || s == "fb"){
    cmd = change_setting_command(self.obj, ts_FillStyle, FillStyle::BORDER_AND_FILL);
  }
  else {
    throw ValueError("Invalid fill-style specified. Valid options are \"border\" (or \"b\"), \"fill\" (or \"f\") or \"border+fill\" (or \"bf\")");
  }

  python_run_command(self, cmd);
}

static int Smth_get_fontsize(const BoundObject& self){
  if (!self.obj->GetSettings().Has(ts_FontSize)){
    throw ValueError("That object does not support fonts");
  }

  const Settings& settings = self.obj->GetSettings();
  return settings.Get(ts_FontSize);
}

static void Smth_set_font(const BoundObject& self, const utf8_string& fontName){
  ObjText* textObj = dynamic_cast<ObjText*>(self.obj);
  if (textObj == nullptr){
    throw ValueError("That object does not support fonts");
  }

  if (!valid_facename(fontName)){
    throw ValueError("Invalid facename"); // Fixme: Better error
  }

  python_run_command(self,
    change_setting_command(self.obj, ts_FontFace, fontName));
}

static utf8_string Smth_get_font(const BoundObject& self){
  ObjText* textObj = dynamic_cast<ObjText*>(self.obj);
  if (textObj == nullptr){
    throw ValueError("That object does not support fonts");
  }
  return textObj->GetSettings().Get(ts_FontFace);
}

static void Smth_set_fontsize(const BoundObject& self, const int& size){
  if (!self.obj->GetSettings().Has(ts_FontSize)){
    throw ValueError("That object does not support fonts");
  }

  python_run_command(self,
    change_setting_command(self.obj, ts_FontSize, size));
}

static int Smth_getObjCount(const BoundObject& self){
  if (!is_composite(self.obj)){
    throw ValueError("That object does not support sub-objects");
  }
  return self.obj->GetObjectCount();
}

static BoundObject Smth_get_sub_object(const BoundObject& self, const int& index){
  if (!is_composite(self.obj)){
    throw ValueError("This object does not support sub-objects");
  }

  if (index < 0 || self.obj->GetObjectCount() <= index ){
    throw ValueError("Invalid object index");
  }
  return BoundObject(self.canvas, self.obj->GetObject(index), self.frameId);
}

static std::vector<coord> flat_point_list(const std::vector<Point>& points){
  std::vector<coord> coords;
  coords.reserve(points.size() * 2);
  for (const Point& pt : points){
    coords.push_back(pt.x);
    coords.push_back(pt.y);
  }
  return coords;
}

static std::vector<coord> Smth_get_points(const BoundObject& self){
  if (is_polygon(self.obj)){
    return flat_point_list(get_polygon_vertices(self.obj));
  }
  else if (is_spline(self.obj)) {
    return flat_point_list(get_spline_points(self.obj));
  }
  else if (is_line(self.obj)){
    return flat_point_list(self.obj->GetMovablePoints());
  }
  else {
    Tri t = self.obj->GetTri();
    return flat_point_list({t.P0(), t.P1(), t.P3(), t.P2()});
  }
}

static utf8_string Smth_get_type(const BoundObject& self){
  return self.obj->GetType();
}

static Tri Smth_tri(const BoundObject& self){
  return self.obj->GetTri();
}

static void Smth_set_tri(const BoundObject& self, const Tri& tri){
  python_run_command(self, new TriCommand(self.obj, New(tri), Old(self.obj->GetTri())));
}

static BoundObject Smth_as_path_object(const BoundObject& self){
  Object* path = clone_as_path(self.obj);
  python_run_command(self, get_replace_object_command(Old(self.obj), path, self.canvas->GetFrameById(self.frameId), select_added(false)));
  return BoundObject(self.canvas, path, self.frameId);
}

// Fixme: Generate all setting-setters. Merge with self-defined functions for like retrieving sub objects etc.
static PyMethodDef smth_methods[] = {
  {"as_path_object", FORWARDER(Smth_as_path_object), METH_VARARGS,
   "as_path_object()\nCreates a path object from this object"},
  {"moveto", FORWARDER(Smth_moveto), METH_VARARGS,
   "Move the thing"},
  {"pos", FORWARDER(Smth_pos), METH_NOARGS,
   "Get the position as a tuple, (x,y)."},
  {"rect", FORWARDER(Smth_rect), METH_NOARGS,
   "Get the bounding rectangle as a tuple, (x,y, width, height)."},
  {"tri", FORWARDER(Smth_tri), METH_NOARGS, "tri()->Tri\nRetrieve a Tri describing the position, skew and angle of the object."},
  {"set_tri", FORWARDER(Smth_set_tri), METH_VARARGS, "set_tri(tri)\nAdjust this object's geometry to the passed in triangle."},
  {"set_angle", FORWARDER(Smth_set_angle), METH_VARARGS, "set_angle(a)\nSets the rotation of this object to a-radians"},
  {"set_angles", FORWARDER(Smth_set_angles), METH_VARARGS, "set_angles(a1, a2)\nSet the interior angles for an elliptic arc in radians."},
  {"set_linewidth", FORWARDER(Smth_set_linewidth), METH_VARARGS,
   "Set the width of the edge line"},
  {"get_linewidth", FORWARDER(Smth_get_linewidth), METH_VARARGS,
   "Retrieve the width of the edge line"},
  {"set_linestyle", FORWARDER(Smth_set_linestyle), METH_VARARGS,
   "Set the style of the line"},
  {"get_linestyle", FORWARDER(Smth_get_linestyle), METH_NOARGS,
   "Get the style of the line"},
  {"set_text", FORWARDER(Smth_set_text), METH_VARARGS,
   "set_text(s)\nSet the text of Text-objects"},
  {"get_png_string", FORWARDER(Smth_get_png_string), METH_NOARGS,
   "get_png_string()->s\nReturns a string of the bitmap encoded as PNG (Raster-objects only)"},
  {"get_settings", FORWARDER(Smth_get_settings), METH_NOARGS, "get_settings()-> settings\nReturns the settings of the object."},
  {"get_text", FORWARDER(Smth_get_text), METH_NOARGS, "get_text()-> s\nReturns the text from Text-objects."},
  {"get_text_lines", FORWARDER(Smth_get_text_lines), METH_NOARGS, "get_text_lines()->[s,...]\nReturns the text split into lines from Text-objects."},
  {"get_text_height", FORWARDER(Smth_get_text_height), METH_NOARGS, "get_text_height()->f\nReturns the text height for this object.\nRelevant only for text objects."},
  {"get_text_baseline", FORWARDER(Smth_get_text_baseline), METH_NOARGS, "get_text_baseline()->f\nReturns the offset from the top of a row to the baseline.\nRelevant only for text objects."},
  {"rotate", FORWARDER(Smth_rotate), METH_VARARGS, "rotate(a)\nRotate this object a-radians clock-wise"},
  {"skew", FORWARDER(Smth_skew), METH_VARARGS, "Skew the thing"},
  {"get_skew", FORWARDER(Smth_get_skew), METH_VARARGS, "Gets the skewness of this object"},
  {"get_angle", FORWARDER(Smth_get_angle), METH_NOARGS, "Returns the angle of the object" },
  {"get_points", FORWARDER(Smth_get_points), METH_NOARGS, "get_points()->(f,f...) returns a list of points for Polygons, Splines and Paths" },
  {"get_fg", FORWARDER(Smth_get_fg), METH_NOARGS, "get_fg()->c\nreturns the foreground as a color tuple, a Gradient or a Pattern" },
  {"get_bg", FORWARDER(Smth_get_bg), METH_NOARGS, "get_bg()->c\nreturns the background as a color tuple, a Gradient or a Pattern" },
  {"set_fg", FORWARDER(Smth_set_fg), METH_VARARGS, "set_fg(c)\nsets the foreground from a color tuple, a Gradient or a Pattern" },
  {"set_bg", FORWARDER(Smth_set_bg), METH_VARARGS, "set_bg(c)\nsets the background from a color tuple, a Gradient or a Pattern" },
  {"set_fillstyle", FORWARDER(Smth_set_fillstyle), METH_VARARGS, "set_fillstyle(s)\nSets the specified fill style" }, // Fixme: Not enough docs, but these should be generated
  {"get_fillstyle", FORWARDER(Smth_get_fillstyle), METH_VARARGS, "Returns the object's fill-style" },
  {"get_arrowhead", FORWARDER(Smth_get_arrowhead), METH_VARARGS, "Returns the arrow-head setting" },
  {"set_font", FORWARDER(Smth_set_font), METH_VARARGS, "Sets the font of the object"},
  {"get_font", FORWARDER(Smth_get_font), METH_NOARGS, "Returns the object's font"},
  {"set_fontsize", FORWARDER(Smth_set_fontsize), METH_VARARGS, "set_fontsize(i)\nSets the point-size of the object's font (for text objects only)" },
  {"get_fontsize", FORWARDER(Smth_get_fontsize), METH_VARARGS, "get_fontsize()->i\nReturns the point-size of the object's font (for text objects only)" },
  {"get_transparency", FORWARDER(Smth_get_transparency), METH_NOARGS, "Returns the object's transparency setting"},
  {"num_objs", FORWARDER(Smth_getObjCount), METH_NOARGS, "num_objs()->i\nReturns the number of sub-objects. Only useful for groups." },
  {"get_obj", FORWARDER(Smth_get_sub_object), METH_VARARGS, "get_obj(i)-> Object\nReturns the sub-object specified by the passed in integer.\nOnly useful for groups."},
  {"get_type", FORWARDER(Smth_get_type), METH_NOARGS, "get_type()->s\nReturns the type of the object (e.g. Rectangle, Ellipse, Line...)" },
  {"update_settings", FORWARDER(Smth_update_settings), METH_VARARGS, "update_settings(s)\nUpdate this object's settings with those from the passed in\nSettings object.\nAny settings not relevant for this object will be ignored"},
  {0,0,0,0}  // Sentinel
};

static int Smth_init(smthObject* self, PyObject*, PyObject*){
  self->obj = nullptr;
  PyErr_SetString(PyExc_TypeError,
    "Something objects can not be created manually. Use the designated object creation functions (e.g. Ellipse) instead.");
  return init_fail;
}

static PyObject* Smth_new(PyTypeObject* type, PyObject* , PyObject*){
  smthObject *self;
  self = (smthObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* Smth_repr(smthObject* self){
  if (!get_app_context().Exists(self->canvasId)){
    return Py_BuildValue("s", "Orphaned object");
  }
  else if (!self->canvas->Has(self->frameId)){
    return Py_BuildValue("s", "Orphaned object");
  }
  const Image& image = get_image(self);
  if (!image.Has(self->objectId)){
    return Py_BuildValue("s", "Removed object");
  }
  else {
    utf8_string str(self->obj->GetType());
    return Py_BuildValue("s", str.c_str()); // Fixme: utf8 c_str bad idea
  }
}

PyTypeObject SmthType = {
  PyVarObject_HEAD_INIT(nullptr,0)
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
  0, // tp_version_tag
  0  // tp_finalize
};

} // namespace
