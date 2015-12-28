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

#include "bitmap/bitmap.hh"
#include "formats/file-formats.hh"
#include "util/canvas.hh"
#include "util/image-util.hh"


namespace faint{

class FormatICO : public Format {
public:
  FormatICO()
    : Format(FileExtension("ico"), label_t(utf8_string("Windows icon (ICO)")), can_save(true), can_load(true))
  {}

  void Load(const FilePath& filePath, ImageProps& imageProps) override{
    load_icon(filePath, imageProps);
  }

  SaveResult Save(const FilePath& filePath, Canvas& canvas) override{
    std::vector<Bitmap> bitmaps;
    int numFrames(canvas.GetNumFrames().Get());
    for (int i = 0; i != numFrames; i++){
      bitmaps.emplace_back(flatten(canvas.GetFrame(index_t(i))));
    }
    return save_icon(filePath, bitmaps);
  }
};

Format* format_ico(){
  return new FormatICO();
}

} // namespace
