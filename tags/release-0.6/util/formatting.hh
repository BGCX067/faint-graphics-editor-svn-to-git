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

#ifndef FAINT_FORMATTING_HH
#define FAINT_FORMATTING_HH
#include <string>
#include "util/commonfwd.hh"

std::string bracketed( const std::string& );
std::string capitalized( const std::string& );
std::string lbl( const std::string& label, const std::string& value );
std::string lbl( const std::string& label, int value);
std::string lowercase( const std::string& );
std::string str( const IntPoint& );
std::string str( const IntSize& );
std::string str( const Point& );
std::string str_axis_adverb( Axis::type );
std::string str_degrees( faint::degree );
std::string str_degrees_int( int );
std::string str_center_radius( const Point&, faint::coord r );
std::string str_center_radius( const Point&, faint::coord rx, faint::coord ry );
std::string str_from_to( const IntPoint&, const IntPoint& );
std::string str_from_to( const Point&, const Point& );
std::string str_hex( const faint::Color& );
std::string str_int_length( int );
std::string str_length( faint::coord );
std::string str_line_status( const Point& p1, const Point& p2 );

// Excludes the alpha-component if fully opaque
std::string str_smart_rgba(const faint::Color&);
std::string str_smart_rgba(const faint::Color&, bool prefix);
std::string str_percentage( int numerator, int denominator );
std::string str_rgb( const faint::Color& );
std::string str_rgba( const faint::Color& );
std::string str_scale( faint::coord scale_x, faint::coord scale_y );

class StrBtn{
// Expresses a mouse button and its opposite as strings.
public:
  StrBtn( int mouse_flag );
  const std::string This( bool capital ) const;
  const std::string Other( bool capital ) const;
private:
  std::string m_btnThis;
  std::string m_btnThat;
};

class Join{
// Joins strings with function-call operator, separating them with the
// constructor argument.
public:
  Join( const std::string& separator );
  std::string operator()(const std::string&, const std::string&) const;
  std::string operator()(const std::string&, const std::string&, const std::string&) const;
  std::string operator()(const std::string&, const std::string&, const std::string&, const std::string&) const;
  std::string operator()(const std::string&, const std::string&, const std::string&, const std::string&, const std::string&) const;
private:
  std::string m_separator;
};

extern const Join space_sep;
extern const Join comma_sep;

#endif
