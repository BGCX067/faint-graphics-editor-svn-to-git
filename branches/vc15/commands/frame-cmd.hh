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

#ifndef FAINT_FRAME_CMD_HH
#define FAINT_FRAME_CMD_HH
#include "util/delay.hh"
#include "util/distinct.hh" // For Order
#include "util/hot-spot.hh"
#include "util/index.hh"

namespace faint{

class Command;
class Image;
class index_t;
class IntPoint;
class IntSize;

Command* add_frame_command(const IntSize&);
Command* add_frame_command(const Image&, const index_t&);
Command* remove_frame_command(const index_t&);
Command* reorder_frame_command(const new_index_t&, const old_index_t&);
Command* swap_frames_command(const index_t&, const index_t&);

using NewDelay = Order<delay_t>::New;
using OldDelay = Order<delay_t>::Old;

Command* set_frame_delay_command(const index_t&,
  const NewDelay&,
  const OldDelay&);

using NewHotSpot = Order<HotSpot>::New;
using OldHotSpot = Order<HotSpot>::Old;

Command* set_frame_hotspot_command(const index_t& frameIndex,
  const NewHotSpot&,
  const OldHotSpot&);

} // namespace

#endif
