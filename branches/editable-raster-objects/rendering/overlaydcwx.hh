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

#ifndef FAINT_OVERLAYDCWX
#define FAINT_OVERLAYDCWX
#include "overlay.hh"
#include "geo/intsize.hh"
#include "geo/rect.hh"
#include "geo/intrect.hh"

class wxDC;

class OverlayDC_WX : public OverlayDC{
public:
  OverlayDC_WX( wxDC&, faint::coord scale, const Rect& imageRect, const IntSize& imageSize );
  void Caret( const LineSegment& ) override;
  void ConstrainPos( const Point& ) override;
  void Corners( const Tri& ) override;
  void ExtensionPoint( const Point& ) override;
  void GridLines( const Grid&, const IntPoint&, const IntSize& ) override;
  void Handles( const Rect& ) override;
  void Handles( const Tri& ) override;
  void HorizontalLine( faint::coord ) override;
  void MovablePoint( const Point& ) override;
  void Parallelogram( const Tri& ) override;
  void Pivot( const Point& ) override;
  void Pixel( const IntPoint& ) override;
  void Rectangle( const Rect& ) override;
  void VerticalLine( faint::coord ) override;
private:
  OverlayDC_WX& operator=(const OverlayDC_WX&);
  wxDC& m_dc;
  Rect m_imageRect;
  IntSize m_imageSize;
  faint::coord m_scale;
};

#endif
