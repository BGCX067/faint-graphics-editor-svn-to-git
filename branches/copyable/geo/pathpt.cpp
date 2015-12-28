// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#include "geo/angle.hh"
#include "geo/geo-func.hh"
#include "geo/pathpt.hh"

namespace faint{

PathPt PathPt::Rotated(const Angle& angle, const Point& origin) const{
  PathPt p2(type, rotate_point(p, angle, origin),
    rotate_point(c, angle, origin),
    rotate_point(d, angle, origin));

  p2.r = r;
  p2.large_arc_flag = large_arc_flag;
  p2.sweep_flag = sweep_flag;
  p2.axis_rotation = axis_rotation + angle.Deg();
  return p2;
}

}
