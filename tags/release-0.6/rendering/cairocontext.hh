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

#ifndef FAINT_CAIROCONTEXT_HH
#define FAINT_CAIROCONTEXT_HH
#include <string>
#include "bitmap/bitmap.hh"
#include "util/commonfwd.hh"
#include "util/settingid.hh"
#include "util/unique.hh"

namespace faint{

BITMAPRETURN cairo_compatible_bitmap( const IntSize& );
BITMAPRETURN cairo_compatible_sub_bitmap( const Bitmap&, const IntRect& );
BITMAPRETURN cairo_skew( Bitmap& bmp, faint::coord skew, faint::coord skew_pixels );
std::string get_cairo_version();
std::string get_pango_version();

class CairoContextImpl;
class CairoContext{
public:
  CairoContext( faint::Bitmap& );
  ~CairoContext();
  void arc( faint::coord xc, faint::coord yc, faint::coord radius, faint::radian angle1, faint::radian angle2 );
  void close_path();
  void curve_to( const Point&, const Point&, const Point& );
  void fill();
  void fill_preserve();
  void line_to( const Point& );
  void move_to( const Point& );
  void pango_text( const Tri&, const utf8_string&, const Settings&);
  Size pango_text_extents( const utf8_string&, const Settings&) const;
  void restore();
  void rotate(faint::radian angle);
  void save();
  void scale( faint::coord, faint::coord );
  void set_dash( const faint::coord* dashes, int num, double offset );
  void set_line_cap(LineCap::type);
  void set_line_join(LineJoin::type);
  void set_source_rgba( const faint::Color& );
  void set_line_width( faint::coord );
  void stroke();
  void translate( const Point& );
private:
  CairoContextImpl* m_impl;
};
}

#endif
