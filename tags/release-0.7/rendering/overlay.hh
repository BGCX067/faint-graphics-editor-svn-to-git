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

#ifndef FAINT_OVERLAY
#define FAINT_OVERLAY
#include <vector>
#include "util/commonfwd.hh"
#include "util/rasterselection.hh"

class OverlayDC{
public:
  virtual ~OverlayDC();
  virtual void Caret( const Line& ) = 0;
  virtual void ConstrainPos( const Point& ) = 0;
  virtual void Corners( const Tri& ) = 0;
  virtual void ExtensionPoint( const Point& ) = 0;
  virtual void GridLines( const Grid&, const IntPoint&, const IntSize& ) = 0;
  virtual void Handles( const Rect& ) = 0;
  virtual void HorizontalLine( faint::coord y ) = 0;
  virtual void MovablePoint( const Point& ) = 0;
  virtual void Pivot( const Point& center ) = 0;
  virtual void Pixel( const IntPoint& ) = 0;
  virtual void Rectangle( const Rect& ) = 0;
  virtual void VerticalLine( faint::coord x ) = 0;
};

class Overlay;

class Overlays{
  // Container for overlay graphics. The various function add overlay
  // graphics, which can be drawn to an OverlayDC with the
  // Paint-method
public:
  ~Overlays();
  void Caret( const Line& );
  void ConstrainPos( const Point& );
  void Corners( const Tri& );
  void ExtensionPoint( const Point& );
  void HorizontalLine( faint::coord y );
  void MovablePoint( const Point& );
  void Paint( OverlayDC& );
  void Pivot( const Point& center );
  void Pixel( const IntPoint& );
  void Rectangle( const Rect& );
  void VerticalLine( faint::coord x );
private:
  std::vector<Overlay*> m_overlays;
};

#endif
