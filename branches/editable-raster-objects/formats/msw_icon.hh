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
#ifndef FAINT_MSW_ICON
#define FAINT_MSW_ICON
#include "bitmap/bitmap.hh"
#include "formats/saveresult.hh"
#include "util/path.hh"

class ImageProps;
namespace faint{
  void load_icon( const FilePath&, ImageProps& );
  SaveResult save_icon( const FilePath&, const std::vector<faint::Bitmap>& );
  void load_cursor( const FilePath&, ImageProps& );
  SaveResult save_cursor( const FilePath&, const std::vector<faint::Bitmap>&, const std::vector<IntPoint>& );
}

#endif
