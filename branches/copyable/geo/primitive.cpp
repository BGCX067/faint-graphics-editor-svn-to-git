// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#include <cassert>
#include <cfloat>
#include <cmath>
#include <type_traits>
#include "geo/primitive.hh"

namespace faint{
static_assert(std::is_same<coord, decltype(DBL_EPSILON)>::value, "Incorrect type of epsilon for coord");

const coord coord_epsilon = DBL_EPSILON;

int ceiled(coord c){
  return static_cast<int>(std::ceil(c));
}

bool coord_eq(coord lhs, coord rhs){
  return fabs(lhs - rhs) <= coord_epsilon;
}

int floored(coord c){
  return static_cast<int>(std::floor(c));
}

size_t to_size_t(int value){
  assert(value >= 0);
  return static_cast<size_t>(value);
}

int resigned(size_t value){
  return static_cast<int>(value);
}

bool rather_zero(coord c){
  return fabs(c) <= coord_epsilon;
}

coord floated(int v){
  return static_cast<coord>(v);
}

int rounded(coord c){
  return static_cast<int>(c < 0 ? c - 0.5 : c + 0.5);
}

int rounded_down(coord c){
  return static_cast<int>(c - 0.5);
}

int truncated(coord c){
  return static_cast<int>(c);
}

} // namespace
