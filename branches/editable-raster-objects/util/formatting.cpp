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

#include <algorithm>
#include <cctype> // toupper
#include <iomanip>
#include <sstream>
#include "geo/radii.hh"
#include "tools/tool.hh" // For LEFT_MOUSE, RIGHT_MOUSE
#include "util/angle.hh"
#include "util/formatting.hh"
#include "util/drawsource.hh"

std::string bracketed( const std::string& s ){
  return "(" + s + ")";
}

std::string capitalized( const std::string& s ){
  if ( s.empty() ){
    return s;
  }
  std::string s2(s);
  s2[0] = static_cast<char>(std::toupper(s2[0]));
  return s2;
}

std::string quoted( const std::string& s ){
  return "\"" + s + "\"";
}

std::string str_two_ints( int i1, int i2 ){
  std::stringstream ss;
  ss << i1 << "," << i2;
  return ss.str();
}

std::string str( const Point& p ){
  return str( floored(p) );
}

std::string str( const IntPoint& p ){
  return str_two_ints(p.x, p.y);
}

std::string str( const IntSize& sz ){
  return str_two_ints( sz.w, sz.h );
}

std::string str( const faint::DrawSource& src ){
  return dispatch(src,
	[](const faint::Color& c){
	  return comma_sep(bracketed(str_smart_rgba(c, rgb_prefix(true))), str_hex(c));},
	[](const faint::Pattern& ){
	  return "Pattern";},
	[](const faint::Gradient&){
	  return "Gradient";});
}

std::string str_axis_adverb( Axis axis ){
  return axis == Axis::HORIZONTAL ? "Horizontally" : "Vertically";
}

std::string str_scale( faint::coord scale_x, faint::coord scale_y ){
  std::stringstream ss;
  ss << int(scale_x*100) << "%, " << int(scale_y*100) << "%";
  return ss.str();
}

std::string str_int( int v ){
  std::stringstream ss;
  ss << v;
  return ss.str();
}

std::string str_interval( const Interval& interval ){
  std::stringstream ss;
  ss << interval.Lower() << "->" << interval.Upper();
  return ss.str();
}

std::string str_range( const ClosedIntRange& range ){
  std::stringstream ss;
  ss << range.Lower() << "->" << range.Upper();
  return ss.str();
}

std::string str_rgb( const faint::Color& c ){
  std::stringstream ss;
  ss << (int)c.r << "," << (int)c.g << "," << (int)c.b;
  return ss.str();
}

std::string str_rgba( const faint::Color& c ){
  std::stringstream ss;
  ss << (int)c.r << "," << (int)c.g << "," << (int)c.b << "," << (int)c.a;
  return ss.str();
}

std::string str_smart_rgba(const faint::Color& col, const rgb_prefix& prefix ){
  if ( !prefix.Get() ){
    return str_smart_rgba(col);
  }
  else {
    return col.a == 255 ?
      "RGB: " + str_rgb( col ) :
      "RGBA: " + str_rgba( col );
  }
}

std::string str_smart_rgba(const faint::Color& col ){
  return col.a == 255 ? str_rgb( col ) : str_rgba( col );
}

std::string str_hex( const faint::Color& col){
  std::stringstream ss;
  ss << "#";
  ss.fill('0');
  ss << std::uppercase << std::hex <<
    std::setw(2) << (int)col.r <<
    std::setw(2) << (int)col.g <<
    std::setw(2) << (int)col.b;
  return ss.str();
}

std::string str_length( faint::coord len ){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << len;
  return ss.str();
}

std::string str_int_length( int len ){
  std::stringstream ss;
  ss << len;
  return ss.str();
}

std::string str_center_radius( const Point& c, const Radii& r ){
  std::stringstream ss;
  ss << "c: " << c.x << "," << c.y << " r: " << r.x << ", " << r.y;
  return ss.str();
}

std::string str_center_radius( const Point& c, double r ){
  std::stringstream ss;
  ss << "c: " << c.x << "," << c.y << " r: " << r;
  return ss.str();
}

std::string str_from_to( const IntPoint& p1, const IntPoint& p2 ){
  std::stringstream ss;
  ss << p1.x << "," << p1.y << "->" << p2.x << "," << p2.y;
  return ss.str();
}

std::string str_from_to( const Point& p1, const Point& p2 ){
  return str_from_to(floored(p1), floored(p2));
}

const unsigned char ascii_degree(0xba);

std::string str_degrees( faint::degree angle ){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << angle << ascii_degree;
  return ss.str();
}

std::string str_degrees_int( int angle ){
  std::stringstream ss;
  if ( angle == 360 ){
    angle = 0;
  }
  ss << angle << ascii_degree;
  return ss.str();
}

std::string str_percentage( int numerator, int denominator ){
  int scale( static_cast<int>( ( 100 * numerator / (double) denominator ) + 0.5 ) );
  std::stringstream ss;
  ss << scale << "%";
  return ss.str();
}

std::string str_line_status_subpixel( const Point& p1, const Point& p2 ){
  faint::coord lineRadius( radius( p1.x, p1.y, p2.x, p2.y ) );
  int lineAngle = static_cast<int>( angle360(p1.x, p1.y, p2.x, p2.y) + 0.5 );

  return comma_sep(str_from_to(p1,p2),
    lbl("length", str_length(lineRadius)),
    lbl("angle", str_degrees_int(lineAngle)));
}

std::string str_line_status( const IntPoint& p1, const IntPoint& p2 ){
  int lineRadius = 1 + // For non-subpixel lines, p1==p2 means length 1
    truncated(radius( p1.x, p1.y, p2.x, p2.y ) );
  int lineAngle = rounded( angle360(p1.x, p1.y, p2.x, p2.y ) );
  return comma_sep(str_from_to(p1,p2),
    lbl("length", str_int_length(lineRadius)),
    lbl("angle", str_degrees_int(lineAngle)));
}

std::string lbl( const std::string& label, const std::string& value ){
  return label + ": " + value;
}

std::string lbl( const std::string& label, int value ){
  std::stringstream ss;
  ss << label << ": " << value;
  return ss.str();
}

std::string str_yh( int y, int h ){
  std::stringstream ss;
  ss << "y: " << y << " h: " << h;
  return ss.str();
}

std::string lowercase( const std::string& src ){
  std::string s2(src);
  std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
  return s2;
}

StrBtn::StrBtn( int mouse_flag ){
  if ( mouse_flag == LEFT_MOUSE ){
    m_btnThis = "left";
    m_btnThat = "right";
  }
  else if ( mouse_flag == RIGHT_MOUSE ){
    m_btnThis = "right";
    m_btnThat = "left";
  }
}

const std::string StrBtn::This( bool capital ) const {
  return capital ? capitalized(m_btnThis) : m_btnThis;
}

const std::string StrBtn::Other( bool capital ) const {
  return capital ? capitalized(m_btnThat) : m_btnThat;
}

Join::Join( const std::string& separator ):
  m_separator(separator)
{}

std::string Join::operator()( const std::vector<std::string>& v ) const{
  if ( v.empty() ){
    return "";
  }
  size_t size = v.size();
  if ( size == 1 ){
    return v.back();
  }
  std::stringstream ss;
  for ( size_t i = 0; i != size - 1; i++ ){
    ss << v[i] << m_separator;
  }
  ss << v.back();
  return ss.str();
}

std::string Join::operator()( const std::string& s1, const std::string& s2 ) const{
  return s1 + m_separator + s2;
}

std::string Join::operator()( const std::string& s1, const std::string& s2, const std::string& s3 ) const{
  return s1 + m_separator + s2 + m_separator + s3;
}

std::string Join::operator()( const std::string& s1, const std::string& s2, const std::string& s3, const std::string& s4 ) const{
  return s1 + m_separator + s2 + m_separator + s3 + m_separator + s4;
}

std::string Join::operator()( const std::string& s1, const std::string& s2, const std::string& s3, const std::string& s4, const std::string& s5 ) const{
  return s1 + m_separator + s2 + m_separator + s3 + m_separator + s4 + m_separator + s5;
}

const Join comma_sep(", ");
const Join endline_sep("\n");
const Join no_sep("");
const Join space_sep(" ");
