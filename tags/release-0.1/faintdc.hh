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
#include <string>
#include <vector>
#include "cairo.h"
#include "geo/tri.hh"
#include "geo/pathpt.hh"
#include "bitmap/bitmap.h"

class FaintSettings;

class FaintDC {
public:
  FaintDC( faint::Bitmap& );
  ~FaintDC();

  // Draw bitmap without alpha blending. This sets the target pixels
  // to the source bitmap color, including the alpha component. The
  // mask-variant excludes maskColor:ed pixels.
  void Bitmap( faint::Bitmap&, const Point& topLeft );
  void Bitmap( faint::Bitmap&, const faint::Color& maskColor, const Point& topLeft );

  // Alpha-blend the bitmap onto the background. The mask-variant excludes
  // maskColor:ed pixels.
  void Blend( faint::Bitmap&, const Point& topLeft );
  void Blend( faint::Bitmap&, const faint::Color& maskColor, const Point& topLeft );

  void Clear( const faint::Color& );
  void Ellipse( const Tri&, const FaintSettings& );
  void Ellipse( faint::coord x, faint::coord y, faint::coord w, faint::coord h, const FaintSettings& );
  faint::Color GetPixel( faint::coord x, faint::coord y ) const;
  // Draws a line from Tri::P0() to Tri::P1()
  void Line( const Tri&, const FaintSettings& );
  void Line( faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1, const FaintSettings& );
  void Lines( const std::vector<IntPoint>&, const FaintSettings& );
  void Path( const std::vector<PathPt>&, const FaintSettings& );
  void Polygon( const std::vector<Point>&, const FaintSettings& );
  void Rectangle( const IntRect&, const FaintSettings& );
  void Rectangle( const Point&, const Point&, const FaintSettings& );
  void Rectangle( const IntPoint&, const IntPoint&, const FaintSettings& );
  void Rectangle( const Tri&, const FaintSettings& );
  void SetOrigin( faint::coord x, faint::coord y );
  void SetScale( faint::coord scale );
  void Spline( const std::vector<Point>&, const FaintSettings& );
  void Stroke( const std::vector<IntPoint>&, const FaintSettings& );

  // Note: Strings must be UTF8
  void Text( const Tri&, const std::string&, const FaintSettings& );
  Size TextExtents( const std::string&, const FaintSettings& ) const;
private:
  FaintDC( const FaintDC& );
  FaintDC& operator=(const FaintDC&);
  void DrawPoint( const Point& );
  void DrawRasterEllipse( const Tri&, const FaintSettings& );
  void DrawRasterRect( const Tri&, const FaintSettings& );
  cairo_t* m_cr;
  cairo_surface_t* m_surface;
  faint::Bitmap& m_bitmap;
  faint::coord m_sc;
  faint::coord m_x0;
  faint::coord m_y0;
};

#endif
