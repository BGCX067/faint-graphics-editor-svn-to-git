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

#ifndef FAINT_DC_HH
#define FAINT_DC_HH
#include <memory>
#include <vector>
#include "geo/intpoint.hh"
#include "geo/point.hh"
#include "rendering/cairo-context.hh"
#include "rendering/filter-class.hh"
#include "text/text-measures.hh"
#include "util/common-fwd.hh"
#include "util/distinct.hh"

namespace faint{
class category_faint_dc;
typedef Distinct<Point, FaintDC, 0> origin_t;

class FaintDC {
public:
  FaintDC(Bitmap&);
  FaintDC(Bitmap&, const origin_t&, coord scale=1.0);

  // True if this context targets pixel-data at the same address as
  // that of the passed in bitmap. This isn't fault proof, and should
  // only be used for extra error checking.
  bool IsTargetting(const Bitmap&) const;

  bool IsOk() const;
  std::string ErrorString() const;
  void Arc(const Tri&, const AngleSpan&, const Settings&);
  void Blit(const Bitmap&, const Point& topLeft, const Settings&);
  void Blend(const AlphaMap&, const IntPoint& topLeft, const IntPoint& anchor, const Settings&);
  void Clear(const Color&);
  void Ellipse(const Tri&, const Settings&);
  Color GetPixel(const Point&) const;
  std::vector<PathPt> GetTextPath(const Tri&, const utf8_string&, const Settings&);
  void Line(const LineSegment&, const Filter&, const Settings&);
  void Path(const std::vector<PathPt>&, const Settings&);
  void PenStroke(const std::vector<IntPoint>&, const Settings&);
  void Polygon(const Tri&, const std::vector<Point>&, const Settings&);
  void PolyLine(const Tri&, const std::vector<Point>&, const Settings&);
  void Rectangle(const Tri&, const Settings&);
  void SetOrigin(const Point&);
  void SetScale(coord);
  void Spline(const std::vector<Point>&, const Settings&);
  void Text(const Tri&, const utf8_string&, const Settings&);
  void Text(const Tri&, const utf8_string&, const Settings&, const Tri& clip);
  Size TextSize(const utf8_string&, const Settings&) const;
  TextMeasures TextExtents(const utf8_string&, const Settings&) const;
  std::vector<int> CumulativeTextWidth(const utf8_string&, const Settings&) const;

  FaintDC(const FaintDC&) = delete;
  FaintDC& operator=(const FaintDC&) = delete;
private:
  void BitmapSetAlpha(const Bitmap&, const IntPoint&);
  void BitmapSetAlphaMasked(const Bitmap&, const Color&, const IntPoint&);
  void BitmapBlendAlpha(const Bitmap&, const IntPoint&);
  void BitmapBlendAlphaMasked(const Bitmap&, const Color&, const IntPoint&);
  void DrawRasterEllipse(const Tri&, const Settings&);
  void DrawRasterEllipse(const Tri&, const Filter&, const Settings&);

  void DrawRasterPolygon(const std::vector<Point>&, const Settings&);
  void DrawRasterPolyLine(const std::vector<Point>&, const Settings&);
  void DrawRasterRect(const Tri&, const Settings&);
  void DrawRasterRect(const Tri&, const Filter&, const Settings&);
  Bitmap& m_bitmap;
  std::unique_ptr<CairoContext> m_cr;
  Point m_origin;
  coord m_sc;
};

} // namespace

#endif
