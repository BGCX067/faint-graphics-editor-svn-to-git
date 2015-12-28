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
#include <vector>
#include "geo/point.hh"
#include "util/commonfwd.hh"
#include "util/unique.hh"

namespace faint{
  class CairoContext;
}

class FaintDC;
typedef Unique<Point, FaintDC, 0> origin_t;

class FaintDC {
public:
  FaintDC( faint::Bitmap& );
  FaintDC( faint::Bitmap&, const origin_t& );
  ~FaintDC();
  void Bitmap( const faint::Bitmap&, const Point& topLeft, const Settings& );
  void Clear( const faint::Color& );
  void Ellipse( const Tri&, const Settings& );
  faint::Color GetPixel( const Point& ) const;
  void Line( const Point&, const Point&, const Settings& );
  void Path( const std::vector<PathPt>&, const Settings& );
  void Polygon( const std::vector<Point>&, const Settings& );
  void PolyLine( const std::vector<Point>&, const Settings& );
  void Rectangle( const Tri&, const Settings& );
  void SetOrigin( const Point& );
  void SetScale( faint::coord );
  void Spline( const std::vector<Point>&, const Settings& );
  void Stroke( const std::vector<IntPoint>&, const Settings& );
  void Text( const Tri&, const faint::utf8_string&, const Settings& );
  Size TextExtents( const faint::utf8_string&, const Settings& ) const;
private:
  FaintDC( const FaintDC& );
  FaintDC& operator=(const FaintDC&);
  void BitmapSetAlpha( const faint::Bitmap&, const Point& );
  void BitmapSetAlphaMasked( const faint::Bitmap&, const faint::Color&, const Point& );
  void BitmapBlendAlpha( const faint::Bitmap&, const Point& );
  void BitmapBlendAlphaMasked( const faint::Bitmap&, const faint::Color&, const Point& );
  void DrawRasterEllipse( const Tri&, const Settings& );
  void DrawRasterPolygon( const std::vector<Point>&, const Settings& );
  void DrawRasterPolyLine( const std::vector<Point>&, const Settings& );
  void DrawRasterRect( const Tri&, const Settings& );
  void DrawRasterLine( const Point&, const Point&, const Settings& );
  void PenStroke( const std::vector<IntPoint>&, const Settings& );
  faint::Bitmap& m_bitmap;
  faint::CairoContext* m_cr;
  Point m_origin;
  faint::coord m_sc;
};

#endif
