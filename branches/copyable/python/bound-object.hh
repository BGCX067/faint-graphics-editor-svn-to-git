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

#ifndef FAINT_BOUND_OBJECT_HH
#define FAINT_BOUND_OBJECT_HH
#include "util/id-types.hh"

namespace faint{
class Canvas;
class Object;

class BoundObject{
  // Connects an Object to the id of the frame that contains it, and
  // to the canvas containing that frame. Used as the target for
  // get_cpp_object for a smthObject*.
public:
  BoundObject(faint::Canvas* in_canvas, Object* in_obj, FrameId in_frameId)
    : canvas(in_canvas),
      obj(in_obj),
      frameId(in_frameId)
  {}
  faint::Canvas* canvas;
  Object* obj;
  FrameId frameId;

  BoundObject& operator=(const BoundObject&) = delete;
};

} // namespace
#endif
