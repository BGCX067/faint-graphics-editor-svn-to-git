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

#include <cfloat>
#include <type_traits>
#include "basic.hh"
#include "intpoint.hh"
#include "point.hh"
namespace faint{
  static_assert(std::is_same<faint::coord, decltype(DBL_EPSILON)>::value, "Incorrect type of epsilon for coord");
  const faint::coord coord_epsilon = DBL_EPSILON;
}

IntPoint round_away_from_zero( const Point& p ){
  return IntPoint( round_away_from_zero(p.x), round_away_from_zero(p.y));
}
