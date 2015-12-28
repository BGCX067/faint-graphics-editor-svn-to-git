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
#include "util/text-measures.hh"
#include "util/unique.hh"

namespace faint{
Bitmap cairo_skew( const Bitmap& bmp, coord skew, coord skew_pixels );
Bitmap cairo_gradient_bitmap( const faint::Gradient&, const IntSize& );
int faint_cairo_stride( const IntSize& );
std::string get_cairo_version();
std::string get_pango_version();

class CairoContextImpl;
class CairoContext{
public:
  CairoContext( faint::Bitmap& );
  ~CairoContext();
  bool IsOk() const;
  std::string ErrorString() const;
  void arc( faint::coord xc, faint::coord yc, faint::coord radius, faint::radian angle1, faint::radian angle2 );
  void close_path();

  // Cubic bezier spline to end using c1 and c2 as control points
  void curve_to( const Point& c1, const Point& c2, const Point& end );
  void fill();
  void fill_preserve();
  void line_to( const Point& );
  void move_to( const Point& );
  void pango_text( const Tri&, const utf8_string&, const Settings&);
  Size pango_text_size( const utf8_string&, const Settings&) const;
  TextMeasures<Rect> pango_text_extents( const utf8_string&, const Settings&) const;
  std::vector<int> cumulative_text_width( const utf8_string&, const Settings&) const; // Fixme: rename, partial widths?
  void restore();
  void rotate(faint::radian angle);
  void save();
  void scale( faint::coord, faint::coord );
  void set_dash( const faint::coord* dashes, int num, double offset );
  void set_line_cap(LineCap);
  void set_line_join(LineJoin);
  void set_source_tri( const Tri& );
  void set_source( const faint::DrawSource& );
  void set_line_width( faint::coord );
  void stroke();
  void translate( const Point& );
private:
  void set_source_surface( const faint::Pattern& );
  void set_source_rgba( const faint::Color& );
  void set_source_gradient( const Gradient& );
  CairoContextImpl* m_impl;
};
}

#endif
