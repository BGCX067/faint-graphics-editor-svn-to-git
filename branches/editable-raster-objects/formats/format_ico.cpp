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
#include "formats/format_ico.hh"
#include "formats/msw_icon.hh"
#include "util/canvasinterface.hh"
#include "util/image.hh"
#include "util/imageutil.hh"

FormatICO::FormatICO()
  : Format( extension_t("ico"), label_t("Windows icon (ICO)"), can_save(true), can_load(true))
{}

void FormatICO::Load( const faint::FilePath& filename, ImageProps& imageProps ){
  faint::load_icon( filename, imageProps );
}

SaveResult FormatICO::Save( const faint::FilePath& filename, CanvasInterface& canvas ){
  std::vector<faint::Bitmap> bitmaps;
  size_t numFrames(canvas.GetNumFrames().Get());
  for ( size_t i = 0; i != numFrames; i++ ){
    bitmaps.push_back(faint::flatten(canvas.GetFrame(index_t(i))));
  }
  return faint::save_icon(filename, bitmaps);
}
