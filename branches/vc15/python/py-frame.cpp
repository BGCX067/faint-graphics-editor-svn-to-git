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

#include <sstream>
#include "app/canvas.hh"
#include "app/frame.hh"
#include "commands/frame-cmd.hh"
#include "commands/put-pixel-cmd.hh"
#include "text/formatting.hh"
#include "util/command-util.hh"
#include "util/image.hh"
#include "util/object-util.hh"
#include "util/setting-util.hh"
#include "python/py-include.hh"
#include "python/py-interface.hh"
#include "python/py-canvas.hh"
#include "python/py-common.hh"
#include "python/py-frame.hh"
#include "python/py-less-common.hh"
#include "python/py-tri.hh"
#include "python/py-ugly-forward.hh"
#include "python/py-util.hh"

namespace faint{

bool expired(frameObject* self){
  if (canvas_ok(self->canvasId) && self->canvas->Has(self->frameId)){
    return false;
  }
  PyErr_SetString(PyExc_ValueError, "That frame is removed."); // Fixme: really do this?
  return true;
}

template<>
struct MappedType<const Frame&>{
  using PYTHON_TYPE = frameObject;

  static Frame GetCppObject(frameObject* self){
    return Frame(self->canvas,
      self->canvas->GetFrame(self->frameId));
  }

  static bool Expired(frameObject* self){
    return expired(self);
  }

  static void ShowError(frameObject*){
    PyErr_SetString(PyExc_ValueError, "That frame is removed.");
  }
};


static PyObject* frame_new(PyTypeObject* type, PyObject*, PyObject*){
  frameObject* self;
  self = (frameObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static PyObject* frame_repr(frameObject* self){
  std::stringstream ss;
  if (canvas_ok(self->canvasId)){
    ss << "Frame of canvas #" << self->canvasId.Raw();
  }
  else {
    ss << "Frame of retired Canvas #" << self->canvasId.Raw();
  }
  return Py_BuildValue("s", ss.str().c_str());
}

static void frame_init(frameObject& self,
  Canvas* canvas,
  const index_t& frameNum)
{
  throw_if_outside(frameNum, canvas->GetNumFrames());

  self.canvas = canvas;
  self.canvasId = canvas->GetId();
  self.frameId = canvas->GetFrame(frameNum).GetId();
}

// Fixme: Consider return bitmap even if ColorSpan?
/* method: "get_bitmap()->bmp?\n
Returns a copy of the pixel data as a Bitmap." */
static const Optional<Bitmap>& frame_get_bitmap(const Frame& frame){
  return frame.image.GetBackground().Get<Bitmap>();
}

/* method: "get_delay()->s/100\n
Returns the duration, in hundredths of a second, that this frame will
be shown when saved as a gif." */
static int frame_get_delay(const Frame& frame){
  return frame.GetImage().GetDelay().Get();
}

/* method: "get_hotspot()-> x,y\n
Returns the hotspot for the frame. This is the anchor point for
cursors (.cur)." */
static IntPoint frame_get_hotspot(const Frame& frame){
  return frame.GetImage().GetHotSpot();
}

/* method: "get_size() -> w,h\n
Returns the size of the frame (in pixels)." */
static IntSize frame_get_size(const Frame& frame){
  return frame.image.GetSize();
}

/* method: "get_objects() -> objects\n
Returns a list of the objects in the frame, sorted from rear-most to
front-most." */
static BoundObjects frame_get_objects(const Frame& frame){
  const objects_t& objects = frame.image.GetObjects();
  auto frameId = frame.frameId;
  BoundObjects boundObjects;
  for (Object* obj : objects){
    boundObjects.emplace_back(frame.canvas, obj, frameId);
  }
  return boundObjects;
}

/* method: "get_calibration() -> ((x0,y0,x1,y1), length, unit)\n
Returns a line, its specified length and the unit this refers to - or
None if the image is not calibrated." */
static Optional<Calibration> frame_get_calibration(const Frame& frame){
  return frame.image.GetCalibration();
}

/* method: "set_delay(s/100)\n
Sets the duration, in hundredths of a second, that this frame will be
shown when saved as a gif." */
static void frame_set_delay(const Frame& frame, int delay){
  if (delay < 0){ // Fixme: -1 should probably mean 'forever' for gifs.
    throw ValueError("Negative delay");
  }

  delay_t oldDelay(frame.GetImage().GetDelay());
  delay_t newDelay(delay);

  python_run_command(frame,
    set_frame_delay_command(frame.GetFrameIndex(),
      New(newDelay), Old(oldDelay)));
}

/* method: "set_hotspot(x,y)\n
Sets the hotspot to x,y. This is the anchor point for cursors
(.cur)." */
static void frame_set_hotspot(const Frame& frame, const HotSpot& pos){
  auto oldHotSpot(frame.GetImage().GetHotSpot());
  python_run_command(frame,
    set_frame_hotspot_command(frame.GetFrameIndex(),
      New(pos), Old(oldHotSpot)));
}

/* method: "set_pixel((x,y),(r,g,b[,a]))\n
Sets the pixel at x,y to the specified color." */
static void frame_set_pixel(const Frame& frame, const IntPoint& pos, const Color& color){
  frame.image.GetBackground().Visit(
    [&](const Bitmap& bmp){
      if (invalid_pixel_pos(pos, bmp)){
        throw ValueError("Invalid pixel position");
      }
      python_run_command(frame, get_put_pixel_command(pos, color));
    },
    [&](const ColorSpan& span){
      if (invalid_pixel_pos(pos, span)){
        throw ValueError("Invalid pixel position");
      }
      python_run_command(frame, get_put_pixel_command(pos, color));
    });
}

using common_type = const Frame&;
/* extra_include: "generated/python/method-def/py-common-methoddef.hh" */
/* extra_include: "generated/python/method-def/py-less-common-methoddef.hh" */

#include "generated/python/method-def/py-frame-methoddef.hh"

PyTypeObject FrameType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "Frame", //tp_name
  sizeof(frameObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0,  // tp_compare
  (reprfunc)frame_repr, // tp_repr
  0, // tp_as_number
  0, // tp_as_sequence
  0, // tp_as_mapping
  0, // tp_hash
  0, // tp_call
  0, // tp_str
  0, // tp_getattro
  0, // tp_setattro
  0, // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
  "An frame in a Faint image.", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  frame_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  INIT_FORWARDER(frame_init), // tp_init
  0, // tp_alloc
  frame_new, // tp_new
  0, // tp_free
  0, // tp_is_gc
  0, // tp_bases
  0, // tp_mro
  0, // tp_cache
  0, // tp_subclasses
  0, // tp_weaklist
  0, // tp_del
  0, // tp_version_tag
  0  // tp_finalize
};

} // namespace
