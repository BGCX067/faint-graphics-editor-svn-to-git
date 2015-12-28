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
class wxDC;

class OverlayDC_WX : public OverlayDC{
public:
  OverlayDC_WX( wxDC& dc, faint::coord scale, const Rect& imageRect, const IntSize& imageSize );
  void Pivot( const Point& center );
  void Rectangle( const Rect& );
  void VerticalLine( faint::coord );
  void HorizontalLine( faint::coord );
  void Caret( const Line& );
  void GridLines( const Grid& );
private:
  OverlayDC_WX& operator=(const OverlayDC_WX&);
  wxDC& m_dc;
  Rect m_imageRect;  
  IntSize m_imageSize;
  faint::coord m_scale;
};

void OverlayPenAndBrush( wxDC& );
#endif
