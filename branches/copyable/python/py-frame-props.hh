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

#ifndef FAINT_PY_FRAME_PROPS_HH
#define FAINT_PY_FRAME_PROPS_HH
#include "python/mapped-type.hh"
#include "util/frame-props.hh"

namespace faint{

class ImageProps;
extern PyTypeObject FramePropsType;

class BoundFrameProps{
public:
  BoundFrameProps(ImageProps&, FrameProps&);
  ImageProps& image;
  FrameProps& frame;
  BoundFrameProps& operator=(const BoundFrameProps&) = delete;
};

typedef struct {
  PyObject_HEAD
  ImageProps* imageProps;
  int frame_index;
} framePropsObject;

template<>
struct MappedType<const BoundFrameProps&>{
  typedef framePropsObject PYTHON_TYPE;
};

BoundFrameProps get_cpp_object(framePropsObject*);
bool faint_side_ok(framePropsObject*);
void show_error(framePropsObject*);

}

#endif
