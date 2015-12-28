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

#ifndef FAINT_AUTO_CROP_HH
#define FAINT_AUTO_CROP_HH
#include "util/template-fwd.hh"

namespace faint{

// Returns true and stores the region to crop to in result if there
// are contiguous colors to crop away at the edges of the bitmap.
// Returns false otherwise - the resulting rectangle will then be
// meaningless.
// Fixme: Consider removing this in favor of get_auto_crop_rectangles,
// however this is still used by ObjRaster.
bool get_auto_crop_rect(const Bitmap&, IntRect& result);

// Returns either zero, one or two regions that can be cropped to.
// The regions are determined by removing contiguous colors at the
// edges. Two regions are possible if one side of the image is all one
// color and the opposite edge is of a different color.
std::vector<IntRect> get_auto_crop_rectangles(const Bitmap&);

// Will return empty vector if optional not set
std::vector<IntRect> get_auto_crop_rectangles(const Optional<Bitmap>&);

// Determines the color under the IntRect edge in the Bitmap.
// Returns an un-set Optional if the edge intersects more than one color.
Optional<Color> get_edge_color(const Bitmap&, const IntRect&);

bool get_bottom_edge_color(const Bitmap&, Color& result);
bool get_left_edge_color(const Bitmap&, Color& result);
bool get_right_edge_color(const Bitmap&, Color& result);
bool get_top_edge_color(const Bitmap&, Color& result);

}
#endif
