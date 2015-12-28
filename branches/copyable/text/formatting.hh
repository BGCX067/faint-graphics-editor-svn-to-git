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

#ifndef FAINT_FORMATTING_HH
#define FAINT_FORMATTING_HH
#include <functional>
#include <string>
#include <vector>
#include "geo/range.hh"
#include "text/utf8-string.hh"
#include "util/common-fwd.hh"

namespace faint{

utf8_string bracketed(const utf8_string&);
utf8_string capitalized(const utf8_string&);
utf8_string lbl(const utf8_string& label, const utf8_string& value);
utf8_string lbl(const utf8_string& label, int value);
utf8_string lowercase(const utf8_string&);
utf8_string quoted(const utf8_string&);
utf8_string str(const IntPoint&);
utf8_string str(const IntSize&);
utf8_string str(const Point&);
utf8_string str(const index_t&);
utf8_string str(const Paint&);
utf8_string str(const Scale&);
utf8_string str_axis_adverb(Axis);
utf8_string str_degrees(const Angle&);
utf8_string str_degrees_symbol(const Angle&);
utf8_string str_degrees_int_symbol(int);
utf8_string str_center_radius(const Point&, coord r);
utf8_string str_center_radius(const Point&, const Radii&);
utf8_string str_from_to(const IntPoint&, const IntPoint&);
utf8_string str_from_to(const Point&, const Point&);
utf8_string str_hex(const Color&);
utf8_string str_interval(const Interval&);
utf8_string str_int_length(int);
utf8_string str_length(coord);
utf8_string str_line_status(const IntLineSegment&);
utf8_string str_line_status_subpixel(const LineSegment&);
utf8_string str_range(const ClosedIntRange&);

// Excludes the alpha-component if fully opaque
utf8_string str_smart_rgba(const Color&);

class category_formatting;
typedef Distinct<bool, category_formatting, 0> rgb_prefix;
utf8_string str_smart_rgba(const Color&, const rgb_prefix&);
utf8_string str_percentage(int numerator, int denominator);
utf8_string str_rgb(const Color&);
utf8_string str_rgba(const Color&);
utf8_string str_int(int);

class StrBtn{
// Expresses a mouse button and its opposite as strings.
public:
  StrBtn(MouseButton);
  const utf8_string This(bool capital) const;
  const utf8_string Other(bool capital) const;
private:
  utf8_string m_btnThis;
  utf8_string m_btnThat;
};

inline utf8_string join(const utf8_string& /*sep*/){
  return utf8_string("");
}

inline utf8_string concat(const utf8_string& l, const utf8_string& r){
  return l + r;
}

template<typename T>
T join(const utf8_string& /*sep*/, const T terminal){
  return terminal;
}

template<class A, class ...B>
utf8_string join(const utf8_string& sep, const A& head, const B&... tail){
  return concat(concat(head, sep), join(sep, tail...));
}

template<class A, class ...B>
utf8_string comma_sep(const A& head, const B&... tail){
  return join(utf8_string(", "), head, tail...);
}

template<class A, class ...B>
utf8_string endline_sep(const A& head, const B&... tail){
  return join(utf8_string("\n"), head, tail...);
}

template<class A, class ...B>
utf8_string no_sep(const A& head, const B&... tail){
  return join(utf8_string(""), head, tail...);
}

template<class A, class ...B>
utf8_string space_sep(const A& head, const B&... tail){
  return join(utf8_string(" "), head, tail...);
}

utf8_string join(const utf8_string& sep, const std::vector<utf8_string>&);
utf8_string comma_sep(const std::vector<utf8_string>&);
utf8_string endline_sep(const std::vector<utf8_string>&);
utf8_string no_sep(const std::vector<utf8_string>&);

// Converts all objects in the container to strings using func, and
// concatenates them separated by the specified separator
template<typename T>
utf8_string join_fn(const utf8_string& separator, T container, const std::function<utf8_string(const typename T::value_type&)>& func){
  if (container.empty()){
    return utf8_string("");
  }

  typename T::const_iterator it = begin(container);
  utf8_string result = func(*it);
  it++;
  while (it != end(container)){
    result += separator;
    result += func(*it);
    it++;
  }
  return result;
}

}

#endif
