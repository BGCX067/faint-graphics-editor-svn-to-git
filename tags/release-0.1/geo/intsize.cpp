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

#include "intsize.hh"

IntSize::IntSize( int in_w, int in_h ) :
  w(in_w),
  h(in_h)
{}

bool IntSize::operator==( const IntSize& other ) const{
  return w == other.w && h == other.h;
}

bool IntSize::operator!=( const IntSize& other ) const{
  return !((*this)==other);
}
