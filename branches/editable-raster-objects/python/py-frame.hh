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

#ifndef FAINT_PY_FRAME_HH
#define FAINT_PY_FRAME_HH
#include "util/commonfwd.hh"
#include "util/idtypes.hh"

// Python-interface to one frame in a Canvas
extern PyTypeObject FrameType;
typedef struct {
  PyObject_HEAD
  CanvasInterface* canvas;
  CanvasId canvasId;
  FrameId frameId; 
} frameObject;

// Returns true if the frame can be used, Otherwise, returns false and
// sets a python error string.
bool frame_ok( frameObject* );
#endif
