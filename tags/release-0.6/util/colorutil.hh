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
#include "geo/geotypes.hh"

class wxBitmap;
// Returns a bitmap of the specified size filled with the given color.
// If the color has alpha, the color will be blended towards a
// checkered pattern.
// Defined in wxutil.cpp
wxBitmap color_bitmap( const faint::Color&, const IntSize&, bool border );

// The hue part of HSL (Fixme: range?)
int get_hue( const faint::Color& );

// The lightness part of HSL (Fixme: range?)
int get_lightness( const faint::Color& );

// The saturation part of HSL (Fixme: range?)
int get_saturation( const faint::Color& );

#endif
