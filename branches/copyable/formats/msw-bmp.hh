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

#ifndef FAINT_MSW_BMP_HH
#define FAINT_MSW_BMP_HH
#include "bitmap/bitmap-fwd.hh"
#include "formats/save-result.hh"
#include "geo/intpoint.hh"
#include "util/file-path.hh"

namespace faint{
  class ImageProps;
  void load_icon(const FilePath&, ImageProps&);
  SaveResult save_icon(const FilePath&, const std::vector<Bitmap>&);
  void load_cursor(const FilePath&, ImageProps&);
  SaveResult save_cursor(const FilePath&, const std::vector<Bitmap>&, const std::vector<IntPoint>&);

  enum class BitmapQuality{COLOR_8BIT, COLOR_24BIT, GRAY_8BIT};
  SaveResult save_bitmap(const FilePath&, const Bitmap&, BitmapQuality);
}

#endif
