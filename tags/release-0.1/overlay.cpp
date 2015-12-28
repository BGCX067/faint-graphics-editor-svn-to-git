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

#include "overlay.hh"
#include <cstddef> // size_t
OverlayDC::~OverlayDC(){}

class Overlay {
public:
  virtual ~Overlay(){}
  virtual void Paint( OverlayDC& ) const = 0;
};

class OverlayPivot : public Overlay {
public:
  OverlayPivot( const Point& p )
    : m_pivot(p)
  {}
  void Paint( OverlayDC& dc ) const{
    dc.Pivot( m_pivot );
  }
private:
  Point m_pivot;
};

class OverlayRectangle : public Overlay {
public:
  OverlayRectangle( const Rect& r )
    : m_rect(r)
  {}
  void Paint( OverlayDC& dc ) const{
    dc.Rectangle( m_rect );
  }
private:
  Rect m_rect;
};

class OverlayVerticalLine : public Overlay {
public:
  OverlayVerticalLine( faint::coord x )
    : m_x(x)
  {}

  void Paint( OverlayDC& dc ) const {
    dc.VerticalLine( m_x );
  }
private:
  faint::coord m_x;
};

class OverlayHorizontalLine : public Overlay {
public:
  OverlayHorizontalLine( faint::coord y )
    : m_y(y)
  {}

  void Paint( OverlayDC& dc ) const{
    dc.HorizontalLine( m_y );
  }
private:
  faint::coord m_y;
};

class OverlayCaret : public Overlay {
public:
  OverlayCaret( const Line& caret )
    : m_caret(caret)
  {}

  void Paint( OverlayDC& dc ) const{
    dc.Caret( m_caret );
  }

private:
  Line m_caret;
};

Overlays::~Overlays(){
  for ( size_t i = 0; i != m_overlays.size(); i++ ){
    delete m_overlays[i];
  }
}

void Overlays::Pivot( const Point& center ){
  m_overlays.push_back( new OverlayPivot( center ) );
}

void Overlays::Rectangle( const Rect& r ){
  m_overlays.push_back( new OverlayRectangle(r) );
}

void Overlays::VerticalLine( faint::coord x ){
  m_overlays.push_back( new OverlayVerticalLine(x) );
}

void Overlays::HorizontalLine( faint::coord y ){
  m_overlays.push_back( new OverlayHorizontalLine(y) );
}

void Overlays::Caret( const Line& line ){
  m_overlays.push_back( new OverlayCaret(line) );
}

void Overlays::Paint( OverlayDC& dc ){
  for ( size_t i = 0; i != m_overlays.size(); i++ ){
    m_overlays[i]->Paint( dc );
  }
}

