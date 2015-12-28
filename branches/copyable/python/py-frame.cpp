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
#include "app/get-app-context.hh"
#include "commands/blit-bitmap-cmd.hh"
#include "commands/draw-object-cmd.hh"
#include "commands/flip-rotate-cmd.hh"
#include "commands/frame-cmd.hh"
#include "commands/put-pixel-cmd.hh"
#include "commands/resize-cmd.hh"
#include "objects/objline.hh"
#include "text/formatting.hh"
#include "util/canvas.hh"
#include "util/command-util.hh"
#include "util/image.hh"
#include "util/object-util.hh"
#include "util/setting-util.hh"
#include "python/py-include.hh"
#include "python/py-interface.hh"
#include "python/py-bitmap.hh"
#include "python/py-canvas.hh"
#include "python/py-common.hh"
#include "python/py-frame.hh"
#include "python/py-tri.hh"
#include "python/py-ugly-forward.hh"
#include "python/py-util.hh"

namespace faint{

void show_error(frameObject*){
  PyErr_SetString(PyExc_ValueError, "That frame is removed.");
}

bool faint_side_ok(frameObject* frame){
  if (canvas_ok(frame->canvasId) && frame->canvas->Has(frame->frameId)){
    return true;
  }
  PyErr_SetString(PyExc_ValueError, "That frame is removed.");
  return false;
}

Frame get_cpp_object(frameObject* self){
  return Frame(self->canvas,
    self->canvas->GetFrameById(self->frameId));
}

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

static int frame_init(frameObject* self, PyObject* args, PyObject*){
  canvasObject* canvas;
  int num;
  if (!PyArg_ParseTuple(args, "O!i", &CanvasType, &canvas, &num)){
    return init_fail;
  }
  if (!canvas_ok(canvas->id)){
    PyErr_SetString(PyExc_ValueError, "Canvas is removed.");
    return init_fail;
  }

  index_t frameCount = canvas->canvas->GetNumFrames();
  if (num < 0 || frameCount <= index_t(num)){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index specified.");
    return init_fail;
  }

  self->canvas = canvas->canvas;
  self->canvasId = canvas->canvas->GetId();
  self->frameId = self->canvas->GetFrame(index_t(num)).GetId();
  return init_ok;
}

static const Optional<Bitmap>& frame_get_bitmap(const Frame& frame){
  return frame.image.GetBg().Get<Bitmap>();
}

static int frame_get_delay(const Frame& frame){
  return frame.GetImage().GetDelay().Get();
}

static IntPoint frame_get_hotspot(const Frame& frame){
  return frame.GetImage().GetHotSpot();
}

static IntSize frame_get_size(const Frame& frame){
  return frame.image.GetSize();
}

static PyObject* frame_get_objects(const Frame& frame){
  return build_object_list(frame.image.GetObjects(), frame.canvas, frame.image.GetId());
}

static void frame_set_delay(const Frame& frame, const int& delay){
  if (delay < 0){ // Fixme: -1 should probably mean 'forever' for gifs.
    throw ValueError("Negative delay");
  }

  delay_t oldDelay(frame.GetImage().GetDelay());
  delay_t newDelay(delay);

  python_run_command(frame,
    set_frame_delay_command(frame.GetFrameIndex(),
      New(newDelay), Old(oldDelay)));
}

static void frame_set_hotspot(const Frame& frame, const IntPoint& pos){
  IntPoint oldHotSpot(frame.GetImage().GetHotSpot());
  python_run_command(frame,
    set_frame_hotspot_command(frame.GetFrameIndex(),
      New(pos), Old(oldHotSpot)));
}

void frame_set_pixel(const Frame& frame, const IntPoint& pos, const Color& color){
  frame.image.GetBg().Visit(
    [&](const Bitmap& bmp){
      if (invalid_pixel_pos(pos, bmp)){
        throw ValueError("Invalid pixel position");
      }
      python_run_command(frame, get_put_pixel_command(pos, color));
    },
    [](const ColorSpan&) {
      // Fixme: This would work, just add invalid_pixel_pos for ColorSpan
      throw ValueError("Invalid pixel position");
    });
}

#define FRAMEFWD(bundle){bundle::Name(), FORWARDER(bundle::Func<const Frame&>), bundle::ArgType(), bundle::Doc()}

// Method table for Python-Frame interface (excluding standard
// functions like del, repr and str) Mind the alphabetical ordering!
static PyMethodDef frame_methods[] = {
  FRAMEFWD(Common_aa_line),
  FRAMEFWD(Common_auto_crop),
  FRAMEFWD(Common_boundary_fill),
  FRAMEFWD(Common_blit),
  FRAMEFWD(Common_clear),
  FRAMEFWD(Common_copy_rect),
  FRAMEFWD(Common_desaturate),
  FRAMEFWD(Common_desaturate_weighted),
  FRAMEFWD(Common_erase_but_color),
  FRAMEFWD(Common_flood_fill),
  {"get_bitmap", FORWARDER(frame_get_bitmap), METH_VARARGS, "get_bitmap()->bmp\nReturns a copy of the pixel data as an ifaint.Bitmap"},
  {"get_delay", FORWARDER(frame_get_delay), METH_VARARGS, "get_delay()->t\nReturns the duration, in hundredths of a second, that this frame will be shown when saved as a gif"},
  {"get_hotspot", FORWARDER(frame_get_hotspot), METH_VARARGS,
   "get_hotspot() -> (x,y)\nReturns the hotspot for the frame."},
  {"get_objects", FORWARDER(frame_get_objects), METH_NOARGS, "get_objects() -> [ object1, object2, ... ]\nReturns the objects in frame"},
  {"get_size", FORWARDER(frame_get_size), METH_NOARGS, "get_size() -> (w,h)\nReturns the size of the frame."},
  FRAMEFWD(Common_insert_bitmap),
  FRAMEFWD(Common_line),
  FRAMEFWD(Common_invert),
  FRAMEFWD(Common_paste),
  FRAMEFWD(Common_pixelize),
  FRAMEFWD(Common_replace_alpha),
  FRAMEFWD(Common_replace_color),
  FRAMEFWD(Common_rotate),
  FRAMEFWD(Common_scale_bilinear),
  FRAMEFWD(Common_scale_nearest),
  FRAMEFWD(Common_set_alpha),
  {"set_delay", FORWARDER(frame_set_delay), METH_VARARGS, "set_delay(delay)\nSets the duration, in hundredths of a second, this frame should be displayed when saved as a .gif"},
  {"set_hotspot", FORWARDER(frame_set_hotspot), METH_VARARGS,
   "set_hotspot((x,y))\nSets the hotspot to the specified point"},
  {"set_pixel", FORWARDER(frame_set_pixel), METH_VARARGS, "hello"},
  FRAMEFWD(Common_set_rect),
  FRAMEFWD(Common_threshold),
  {0,0,0,0}
};

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
  (initproc)frame_init, // tp_init
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
