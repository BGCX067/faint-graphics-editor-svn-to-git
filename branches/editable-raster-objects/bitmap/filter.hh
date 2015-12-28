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

#ifndef FAINT_FILTER_HH
#define FAINT_FILTER_HH
#include "bitmap/bitmap.hh"
#include "bitmap/histogram.hh"
class Filter;

namespace faint{
  struct brightness_contrast_t{
    // Parameters for apply_brightness_and_contrast. This is applied
    // like a gain and bias, with brightness as bias and contrast as
    // gain.
    brightness_contrast_t(); // Initializes to nop-values
    brightness_contrast_t( double brightness, double contrast );
    double brightness;
    double contrast;
  };

  void apply_brightness_and_contrast( faint::Bitmap&, const brightness_contrast_t& );
  void blur(faint::Bitmap&);
  void desaturate_simple( Bitmap& );
  void desaturate_weighted( Bitmap& );
  void pixelize( Bitmap& bmp, int w );
  Filter* get_blur_filter();
  Filter* get_stroke_filter();
  void invert( Bitmap& );
  typedef DefinedIntRange<0,765> threshold_range_t;
  void threshold( Bitmap&, const threshold_range_t& );
  std::vector<int> threshold_histogram( Bitmap& );
}

#endif
