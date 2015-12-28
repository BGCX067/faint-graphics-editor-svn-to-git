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

#include "geo/size.hh"

namespace faint{

Size::Size( faint::coord in_w, faint::coord in_h )
  : w(in_w),
    h(in_h)
{}

Size Size::Both(faint::coord v){
  return Size(v,v);
}

bool Size::operator==( const Size& other ) const {
  return coord_eq(w, other.w) && coord_eq(h, other.h);
}

bool Size::operator!=( const Size& other ) const {
    return !((*this)==other);
}

Size operator+(const Size& lhs, const Size& rhs){
  return Size(lhs.w + rhs.w, lhs.h + rhs.h);
}

Size Size::operator/(const Size& other ) const{
  return Size(w / other.w, h / other.h);
}

Size operator-( const Size& lhs, const Size& rhs ){
  return Size(lhs.w - rhs.w, lhs.h - rhs.h);
}

Size operator/( const Size& lhs, faint::coord scalar ){
  return Size(lhs.w / scalar, lhs.h / scalar);
}

} // namespace
