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

#ifndef FAINT_COLORUTIL_HH
#define FAINT_COLORUTIL_HH
#include "bitmap/bitmap.hh"
#include "geo/geotypes.hh"
#include "util/unique.hh"
#include "util/gradient.hh"
#include "util/pattern.hh"

namespace faint{

  // Returns a bitmap filled with the specified color, with increasing
  // opacity along the y-axis
  Bitmap alpha_gradient_bitmap( const ColRGB&, const IntSize& );

  // Returns a bitmap of the specified size filled with the given
  // color.  If the color has alpha, the color will be blended towards
  // a checkered pattern.
  faint::Bitmap color_bitmap( const faint::Color&, const IntSize& );
  faint::Bitmap color_bitmap( const faint::Color&, const IntSize&, const IntSize& patternSize );
  faint::Bitmap draw_source_bitmap( const faint::DrawSource&, const IntSize& );
  faint::Bitmap draw_source_bitmap( const faint::DrawSource&, const IntSize&, const IntSize& patternSize );
  // Returns a pattern for initializing the color dialogs pattern tab
  faint::Bitmap default_pattern( const IntSize& );

  faint::Bitmap gradient_bitmap( const faint::Gradient&, const IntSize& );

  // Returns a bitmap of hue and saturation values for mid-lightness
  // with hue increasing along the X-axis and saturation decreasing
  // along the Y-axis
  faint::Bitmap hue_saturation_color_map( const IntSize& );

  // Returns a bitmap of the given hue and saturation, with lightness
  // increasing from 0.0 to 1.0 along the y-axis.
  faint::Bitmap lightness_gradient_bitmap( const faint::HS&, const IntSize& );

  // Returns a copy of the bitmap with a 1-pixel thick border.
  faint::Bitmap with_border( const faint::Bitmap& );
}
#endif
