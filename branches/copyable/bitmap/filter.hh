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

#ifndef FAINT_FILTER_HH
#define FAINT_FILTER_HH
#include <vector>
#include "bitmap/bitmap-fwd.hh"
#include "geo/geo-fwd.hh"
#include "geo/range.hh"
#include "util/paint-fwd.hh"

namespace faint{

class Filter;

class brightness_contrast_t{
  // Parameters for apply_brightness_and_contrast. This is applied
  // like a gain and bias, with brightness as bias and contrast as
  // gain.
public:
  // Initializes to nop-values
  brightness_contrast_t();
  brightness_contrast_t(double brightness, double contrast);
  double brightness;
  double contrast;
};

void apply_brightness_and_contrast(Bitmap&, const brightness_contrast_t&);
void blur(Bitmap&);
void desaturate_simple(Bitmap&);
void desaturate_weighted(Bitmap&);
void gaussian_blur(Bitmap&, double sigma);
void pixelize(Bitmap& bmp, int w);
void sepia(Bitmap& bmp, int intensity);
void unsharp_mask(Bitmap& bmp, double blurSigma);
Filter* get_blur_filter();
Filter* get_shadow_filter();
Filter* get_invert_filter();
Filter* get_pinch_whirl_filter();
Filter* get_pixelize_filter();
Filter* get_stroke_filter();
void invert(Bitmap&);

typedef StaticBoundedInterval<0,255> color_range_t;

void color_balance(Bitmap&, const color_range_t& r, const color_range_t& g, const color_range_t& b);

void filter_pinch_whirl(Bitmap& bmp, coord pinch, const Rotation& whirl);

typedef StaticBoundedInterval<0,765> threshold_range_t;
void threshold(Bitmap&, const threshold_range_t&,
  const Paint& in, const Paint& out);
std::vector<int> threshold_histogram(Bitmap&);
std::vector<int> red_histogram(Bitmap&);
std::vector<int> green_histogram(Bitmap&);
std::vector<int> blue_histogram(Bitmap&);
} // namespace

#endif
