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

#include "formats/file-formats.hh"
#include "util/canvas.hh"
#include "util/image.hh"
#include "util/image-util.hh"

namespace faint{

class FormatCUR : public Format {
public:
  FormatCUR()
    : Format(FileExtension("cur"), label_t(utf8_string("Windows cursor (CUR)")), can_save(true), can_load(true))
  {}

  void Load(const FilePath& filename, ImageProps& imageProps){
    load_cursor(filename, imageProps);
  }

  SaveResult Save(const FilePath& filename, Canvas& canvas){
    std::vector<Bitmap> bitmaps;
    std::vector<IntPoint> hotSpots;
    const int numFrames(canvas.GetNumFrames().Get());
    for (int i = 0; i != numFrames; i++){
      const Image& frame(canvas.GetFrame(index_t(i)));
      bitmaps.emplace_back(flatten(frame));
      hotSpots.emplace_back(frame.GetHotSpot());
    }
    return save_cursor(filename, bitmaps, hotSpots);
  }
};

Format* format_cur(){
  return new FormatCUR();
}

} // namespace
