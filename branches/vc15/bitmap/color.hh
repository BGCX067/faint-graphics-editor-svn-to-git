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

#ifndef FAINT_COLOR_HH
#define FAINT_COLOR_HH

namespace faint{

using uchar = unsigned char;
using uint = unsigned int;

class ColRGB{
  // RGB color, i.e. no alpha component, for when that distinction
  // matters - otherwise plays second fiddle to faint::Color. The
  // Col-prefix is there to avoid clashing with the Windows header
  // RGB-macro which sometimes shows up uninvited.
public:
  ColRGB();
  ColRGB(uchar r, uchar g, uchar b);
  bool operator==(const ColRGB&) const;
  bool operator!=(const ColRGB&) const;
  uchar r;
  uchar g;
  uchar b;
};

class Color{
  // RGBA-Color.
public:
  Color();
  Color(uchar r, uchar g, uchar b, uchar a = 255);
  Color(const ColRGB&, uchar a);
  bool operator==(const Color&) const;
  bool operator!=(const Color&) const;

  // Simple field-by-field less for containers.
  bool operator<(const Color& c) const;
  uchar r;
  uchar g;
  uchar b;
  uchar a;
};

class HS{
public:
  HS(double h, double s);
  double h;
  double s;
};

class HSL{
public:
  HSL(double h, double s, double l);
  HSL(const HS&, double l);
  HS GetHS() const;
  double h;
  double s;
  double l;
};

// Creates a Color object from int values. Clamps the values to the
// allowed range.
Color color_from_ints(int r, int g, int b, int a=255);
ColRGB grayscale_rgb(int lightness);
Color grayscale_rgba(int lightness, int a=255);
ColRGB rgb_from_ints(int r, int g, int b);
Color color_from_uints(uint r, uint g, uint b, uint a=255);
Color color_from_double(double r, double g, double b, double a=255.0);
ColRGB rgb_from_double(double r, double g, double b);

ColRGB blend_alpha(const Color&, const ColRGB& background);
ColRGB strip_alpha(const Color&);

ColRGB invert(const ColRGB&);
HSL to_hsl(const ColRGB&);
ColRGB to_rgb(const HSL&);
Color to_rgba(const HSL&, int a=255);
bool opaque(const Color&);
bool translucent(const Color&);
bool valid_color(int r, int g, int b, int a);
bool valid_color(int r, int g, int b);
Color color_black();
Color color_blue();
Color color_gray();
Color color_green();
Color color_magenta();
Color color_red();
Color color_transparent_black();
Color color_transparent_white();
Color color_white();
ColRGB rgb_black();
ColRGB rgb_white();

Color subtract(const Color& lhs, const Color& rhs);
Color add(const Color& lhs, const Color& rhs);
int sum_rgb(const Color&);

unsigned int to_hash(const Color&);
Color color_from_hash(unsigned int);

} // namespace

#endif
