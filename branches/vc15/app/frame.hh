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

#ifndef FAINT_FRAME_HH
#define FAINT_FRAME_HH
#include "app/canvas.hh"
#include "util/id-types.hh"

namespace faint{
class Image;

class Frame{
public:
  Frame(Canvas*, const Image&);
  const Either<Bitmap, ColorSpan>& GetBackground() const;
  index_t GetFrameIndex() const;
  const Image& GetImage() const;
  const RasterSelection& GetRasterSelection() const;
  Frame& operator=(const Frame&) = delete;

  Canvas* canvas;
  const Image& image;
  FrameId frameId;
private:
};

bool contains_pos(const Frame&, const IntPoint&);

} // namespace

#endif
