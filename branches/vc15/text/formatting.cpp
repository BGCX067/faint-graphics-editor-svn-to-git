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

#include <iomanip>
#include <sstream>
#include "bitmap/color.hh"
#include "bitmap/paint.hh"
#include "geo/angle.hh"
#include "geo/axis.hh"
#include "geo/geo-func.hh"
#include "geo/int-size.hh"
#include "geo/line.hh"
#include "geo/measure.hh"
#include "geo/radii.hh"
#include "geo/range.hh"
#include "geo/scale.hh"
#include "text/char-constants.hh"
#include "text/formatting.hh"
#include "util/index.hh"
#include "util/pos-info-constants.hh" // For MouseButton

namespace faint{

utf8_string bracketed(const utf8_string& s){
  return utf8_string("(") + s + utf8_string(")");
}

utf8_string capitalized(const utf8_string& s){
  if (s.empty()){
    return s;
  }
  return toupper(s[0]) + s.substr(1);
}

utf8_string decapitalized(const utf8_string& s){
  if (s.empty()){
    return s;
  }
  return tolower(s[0]) + s.substr(1);
}

utf8_string quoted(const utf8_string& s){
  return utf8_string("\"") + s + utf8_string("\"");
}

utf8_string str_two_ints(int i1, int i2){
  std::stringstream ss;
  ss << i1 << "," << i2;
  return utf8_string(ss.str());
}

utf8_string str(const Point& p){
  // Floor to get rid of decimals and indicate the targeted pixel.
  return str(floored(p));
}

utf8_string str(const IntPoint& p){
  return str_two_ints(p.x, p.y);
}

utf8_string str(const IntSize& sz){
  return str_two_ints(sz.w, sz.h);
}

utf8_string str_float(int precision, coord v){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(precision) << v;
  return utf8_string(ss.str());
}

utf8_string str_user(const index_t& index){
  return str_int(index.Get() + 1);
}

utf8_string str_coder(const index_t& index){
  return str_int(index.Get());
}

utf8_string str(const Paint& paint){
  return dispatch(paint,
    [](const Color& c){
      return comma_sep(bracketed(str_smart_rgba(c, rgb_prefix(true))), str_hex(c));},
    [](const Pattern&){
      return utf8_string("Pattern");},
    [](const Gradient&){
      return utf8_string("Gradient");});
}

utf8_string str(const Scale& scale){
  std::stringstream ss;
  int sc_x = rounded(scale.x * 100);
  int sc_y = rounded(scale.y * 100);
  ss << sc_x << "%";
  if (sc_x != sc_y){
    ss << ", " << sc_y << "%";
  }
  return utf8_string(ss.str());
}

utf8_string str_axis_adverb(Axis axis){
  return axis == Axis::HORIZONTAL ?
    utf8_string("Horizontally") :
    utf8_string("Vertically");
}

utf8_string str_int(int v){
  std::stringstream ss;
  ss << v;
  return utf8_string(ss.str());
}

utf8_string str_int_lpad(int v, int w){
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(w) << std::right;
  ss << v;
  return utf8_string(ss.str());
}

utf8_string str_uint(size_t v){
  std::stringstream ss;
  ss << v;
  return utf8_string(ss.str());
}

utf8_string str_interval(const Interval& interval){
  std::stringstream ss;
  ss << interval.Lower() << "->" << interval.Upper();
  return utf8_string(ss.str());
}

utf8_string str_range(const ClosedIntRange& range){
  std::stringstream ss;
  ss << range.Lower() << "->" << range.Upper();
  return utf8_string(ss.str());
}

utf8_string str_rgb(const Color& c){
  std::stringstream ss;
  ss << (int)c.r << "," << (int)c.g << "," << (int)c.b;
  return utf8_string(ss.str());
}

utf8_string str_rgba(const Color& c){
  std::stringstream ss;
  ss << (int)c.r << "," << (int)c.g << "," << (int)c.b << "," << (int)c.a;
  return utf8_string(ss.str());
}

utf8_string str_smart_rgba(const Color& col, const rgb_prefix& prefix){
  if (!prefix.Get()){
    return str_smart_rgba(col);
  }
  else {
    return col.a == 255 ?
      utf8_string("RGB: ") + str_rgb(col) :
      utf8_string("RGBA: ") + str_rgba(col);
  }
}

utf8_string str_smart_rgba(const Color& col){
  return col.a == 255 ? str_rgb(col) : str_rgba(col);
}

utf8_string str_hex(const Color& col){
  std::stringstream ss;
  ss << "#";
  ss.fill('0');
  ss << std::uppercase << std::hex <<
    std::setw(2) << (int)col.r <<
    std::setw(2) << (int)col.g <<
    std::setw(2) << (int)col.b;
  return utf8_string(ss.str());
}

utf8_string str_hex(const int value){
  std::stringstream ss;
  ss << std::hex << value;
  return utf8_string(ss.str());
}

utf8_string str_length(coord len){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << len;
  return utf8_string(ss.str());
}

utf8_string str_int_length(int len){
  return str_int(len);
}

utf8_string str_center_radius(const Point& c, const Radii& r){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) <<
    "c: " << c.x << "," << c.y << " r: " << r.x << "," << r.y;
  return utf8_string(ss.str());
}

utf8_string str_center_radius(const Point& c, double r){
  std::stringstream ss;

  ss << std::fixed << std::setprecision(1) <<
    "c: " << c.x << "," << c.y << " r: " << r;
  return utf8_string(ss.str());
}

utf8_string str_from_to(const IntPoint& p1, const IntPoint& p2){
  std::stringstream ss;
  ss << p1.x << "," << p1.y << "->" << p2.x << "," << p2.y;
  return utf8_string(ss.str());
}

utf8_string str_from_to(const Point& p1, const Point& p2){
  return str_from_to(floored(p1), floored(p2));
}

utf8_string str_degrees(const Angle& angle){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << angle.Deg();
  return utf8_string(ss.str());
}

utf8_string str_degrees_symbol(const Angle& angle){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << angle.Deg() << degree_sign.str();
  return utf8_string(ss.str());
}

utf8_string str_degrees_int_symbol(int angle){
  std::stringstream ss;
  if (angle == 360){
    angle = 0;
  }
  ss << angle << degree_sign.str();
  return utf8_string(ss.str());
}

utf8_string str_percentage(int numerator, int denominator){
  int scale(static_cast<int>((100 * numerator / (double) denominator) + 0.5));
  std::stringstream ss;
  ss << scale << "%";
  return utf8_string(ss.str());
}

utf8_string str_line_status_subpixel(const LineSegment& l){
  int angle = static_cast<int>(angle360(l).Deg() + 0.5);

  return comma_sep(str_from_to(l.p0,l.p1),
    lbl(utf8_string("length"), str_length(length(l))),
    lbl(utf8_string("angle"), str_degrees_int_symbol(angle)));
}

static utf8_string str_from_to(const IntLineSegment& l){
  return str_from_to(l.p0, l.p1);
}

utf8_string str_line_status(const IntLineSegment& line){
  int lineRadius = 1 + // For non-subpixel lines, p1==p2 means length 1
    truncated(length(line));

  return comma_sep(str_from_to(line),
    lbl("length",
      str_int_length(lineRadius)),

    lbl("angle",
      str_degrees_int_symbol(rounded(angle360(floated(line)).Deg()))));
}

utf8_string lbl(const utf8_string& label, const utf8_string& value){
  return label + utf8_string(": ") + value;
}

utf8_string lbl(const utf8_string& label, int value){
  std::stringstream ss;
  ss << ": " << value;
  return label + utf8_string(ss.str());
}

utf8_string str_yh(int y, int h){
  std::stringstream ss;
  ss << "y: " << y << " h: " << h;
  return utf8_string(ss.str());
}

StrBtn::StrBtn(MouseButton button){
  if (button == MouseButton::LEFT){
    m_btnThis = utf8_string("left");
    m_btnThat = utf8_string("right");
  }
  else if (button == MouseButton::RIGHT){
    m_btnThis = utf8_string("right");
    m_btnThat = utf8_string("left");
  }
}

const utf8_string StrBtn::This(bool capital) const {
  return capital ? capitalized(m_btnThis) : m_btnThis;
}

const utf8_string StrBtn::Other(bool capital) const {
  return capital ? capitalized(m_btnThat) : m_btnThat;
}

utf8_string join(const utf8_string& sep, const std::vector<utf8_string>& strings){
  if (strings.empty()){
    return utf8_string("");
  }

  utf8_string result = strings[0];
  for (size_t i = 1; i != strings.size(); i++){
    result += sep;
    result += strings[i];
  }
  return result;
}

utf8_string comma_sep(const std::vector<utf8_string>& strings){
  return join(utf8_string(", "), strings);
}

utf8_string endline_sep(const std::vector<utf8_string>& strings){
  return join(utf8_string("\n"), strings);
}

utf8_string no_sep(const std::vector<utf8_string>& strings){
  return join(utf8_string(""), strings);
}

} // namespace
