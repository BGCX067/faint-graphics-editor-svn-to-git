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

#ifndef FAINT_SET_BITMAP_CMD_HH
#define FAINT_SET_BITMAP_CMD_HH
#include "commands/command.hh"

namespace faint{

class ObjRaster;

// Sets the image bitmap and offsets any objects using topLeft.
Command* set_bitmap_command(const faint::Bitmap&, const IntPoint& topLeft, const faint::utf8_string& name);

// Sets the bitmap used by the raster object.
Command* set_object_bitmap_command(ObjRaster*, const faint::Bitmap&, const Tri&, const faint::utf8_string& name );

} // namespace

#endif
