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

#ifndef FAINT_RESCALE_CMD_HH
#define FAINT_RESCALE_CMD_HH

namespace faint{

enum class ScaleQuality;
class Command;
class IntSize;

// Scale the image to the specified size. The raster background will
// be scaled with the specified quality. All objects will be scaled
// proportionally.
Command* rescale_command(const IntSize&, ScaleQuality);

}

#endif
