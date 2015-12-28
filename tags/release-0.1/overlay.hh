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
#include "geo/geotypes.hh"
#include "geo/grid.hh"

class OverlayDC{
public:
  virtual ~OverlayDC();
  virtual void Pivot( const Point& center ) = 0;
  virtual void Rectangle( const Rect& ) = 0;
  virtual void VerticalLine( faint::coord x ) = 0;
  virtual void HorizontalLine( faint::coord y ) = 0;
  virtual void Caret( const Line& ) = 0;
  virtual void GridLines( const Grid& ) = 0;
};

class Overlay;

class Overlays{
public:
  ~Overlays();
  void Pivot( const Point& center );
  void Rectangle( const Rect& );
  void VerticalLine( faint::coord x );
  void HorizontalLine( faint::coord y );
  void Caret( const Line& );
  void Paint( OverlayDC& );

private:
  std::vector<Overlay*> m_overlays;
};

#endif
