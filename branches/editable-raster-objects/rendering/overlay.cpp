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

#include <cstddef> // size_t
#include "geo/geotypes.hh"
#include "geo/tri.hh"
#include "rendering/overlay.hh"

OverlayDC::~OverlayDC(){}

class Overlay {
public:
  virtual ~Overlay(){}
  virtual void Paint( OverlayDC& ) const = 0;
};

class OverlayCaret : public Overlay {
public:
  OverlayCaret( const LineSegment& caret )
    : m_caret(caret)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.Caret( m_caret );
  }
private:
  LineSegment m_caret;
};

class OverlayConstrainPos : public Overlay {
public:
  OverlayConstrainPos( const Point& pos )
    : m_pos(pos)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.ConstrainPos(m_pos);
  }
private:
  Point m_pos;
};

class OverlayCorners : public Overlay {
public:
  OverlayCorners( const Tri& tri )
    : m_tri(tri)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.Corners( m_tri );
  }
private:
  Tri m_tri;
};

class OverlayExtension : public Overlay {
public:
  OverlayExtension( const Point& point )
    : m_point(point)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.ExtensionPoint( m_point );
  }
private:
  Point m_point;
};

class OverlayHorizontalLine : public Overlay {
public:
  OverlayHorizontalLine( faint::coord y )
    : m_y(y)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.HorizontalLine( m_y );
  }
private:
  faint::coord m_y;
};

class OverlayMovable : public Overlay {
public:
  OverlayMovable( const Point& point )
    : m_point(point)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.MovablePoint( m_point );
  }
private:
  Point m_point;
};

class OverlayPivot : public Overlay {
public:
  OverlayPivot( const Point& p )
    : m_pivot(p)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.Pivot( m_pivot );
  }
private:
  Point m_pivot;
};

class OverlayPixel : public Overlay {
public:
  OverlayPixel( const IntPoint& p )
    : m_pos(p)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.Pixel( m_pos );
  }
private:
  IntPoint m_pos;
};

class OverlayRectangle : public Overlay {
public:
  OverlayRectangle( const Rect& r )
    : m_rect(r)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.Rectangle( m_rect );
  }
private:
  Rect m_rect;
};

class OverlayParallelogram : public Overlay {
public:
  OverlayParallelogram( const Tri& tri )
    : m_tri(tri)
  {}
  void Paint( OverlayDC& dc ) const override{
    dc.Parallelogram( m_tri );
  }
private:
  Tri m_tri;
};

class OverlayVerticalLine : public Overlay {
public:
  OverlayVerticalLine( faint::coord x )
    : m_x(x)
  {}
  void Paint( OverlayDC& dc ) const override {
    dc.VerticalLine( m_x );
  }
private:
  faint::coord m_x;
};

Overlays::~Overlays(){
  for ( size_t i = 0; i != m_overlays.size(); i++ ){
    delete m_overlays[i];
  }
}

void Overlays::Caret( const LineSegment& line ){
  m_overlays.push_back( new OverlayCaret(line) );
}

void Overlays::ConstrainPos( const Point& pos ){
  m_overlays.push_back( new OverlayConstrainPos(pos) );
}

void Overlays::Corners( const Tri& tri ){
  m_overlays.push_back( new OverlayCorners(tri) );
}

void Overlays::ExtensionPoint( const Point& center ){
  m_overlays.push_back( new OverlayExtension(center) );
}

void Overlays::HorizontalLine( faint::coord y ){
  m_overlays.push_back( new OverlayHorizontalLine(y) );
}

void Overlays::MovablePoint( const Point& center ){
  m_overlays.push_back( new OverlayMovable(center) );
}

void Overlays::Paint( OverlayDC& dc ){
  for ( size_t i = 0; i != m_overlays.size(); i++ ){
    m_overlays[i]->Paint( dc );
  }
}

void Overlays::Parallelogram( const Tri& tri ){
  m_overlays.push_back(new OverlayParallelogram(tri));
}

void Overlays::Pivot( const Point& center ){
  m_overlays.push_back( new OverlayPivot( center ) );
}

void Overlays::Pixel( const IntPoint& pos ){
  m_overlays.push_back( new OverlayPixel( pos ) );
}

void Overlays::Rectangle( const Rect& r ){
  m_overlays.push_back( new OverlayRectangle(r) );
}

void Overlays::VerticalLine( faint::coord x ){
  m_overlays.push_back( new OverlayVerticalLine(x) );
}
