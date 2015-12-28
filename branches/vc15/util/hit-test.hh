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

#ifndef FAINT_HIT_TEST_HH
#define FAINT_HIT_TEST_HH
#include "geo/canvas-geo.hh"
#include "util/pos-info.hh"

namespace faint{

ObjectInfo hit_test(const IntPoint& viewPt,
  const Image&,
  const CanvasGeo&,
  int objectHandleWidth);

} // namespace

#endif