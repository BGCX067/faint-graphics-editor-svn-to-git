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

#ifndef FAINT_ROTATE_UTIL_HH
#define FAINT_ROTATE_UTIL_HH
#include "geo/geo-fwd.hh"
#include "geo/int-point.hh"
#include "geo/int-size.hh"

namespace faint{

class RotationAdjustment{
public:
  RotationAdjustment(const IntPoint&, const IntSize&);
  // Some offset possibly meaningful to rotation algorithms
  IntPoint offset;

  // The required size for a new image to contain a rotated bounding
  // box
  IntSize size;
};

RotationAdjustment get_rotation_adjustment(const Angle&, const IntSize&);

}

#endif
