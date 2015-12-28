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

#include "wx/utils.h"
#include "wx/window.h"
#include "geo/geotypes.hh"
#include "geo/canvasgeo.hh"
#include "convertwx.hh"
#include "mouse.hh"

namespace faint {
  namespace mouse {
    IntPoint ScreenPosition(){
      return to_faint( wxGetMousePosition() );
    }

    IntPoint ViewPosition( const wxWindow& w ){
      return to_faint( w.ScreenToClient( wxGetMousePosition() ) );
    }

    Point ImagePosition( const Geometry& g, const wxWindow& w ){
      Point p( floated(ViewPosition(w)) );
      const faint::coord zoom = g.zoom.GetScaleFactor();
      return Point(
        ( p.x + g.x0 - g.left_border ) / zoom,
        ( p.y + g.y0 - g.top_border ) / zoom );
    }

    Point ViewToImage( const IntPoint& p, const Geometry& g ){
      const faint::coord zoom = g.zoom.GetScaleFactor();
      Point ptImg( (p.x + g.x0 - g.left_border ) / zoom,
        (p.y + g.y0 - g.top_border ) / zoom );
      return ptImg;
    }

    IntPoint ImageToView( const IntPoint& p, const Geometry& g ){
      const faint::coord zoom = g.zoom.GetScaleFactor();
      float x2 = p.x * zoom - g.x0 + g.left_border;
      float y2 = p.y * zoom - g.y0 + g.top_border;
      return IntPoint( x2, y2 ); // Fixme: No rounding, but I guess that is correct...
    }

    IntRect ImageToView( const IntRect& r, const Geometry& g ){
      IntPoint p1 = ImageToView_tr( Point( r.x, r.y ), g );
      IntPoint p2 = ImageToView_tr( Point( r.Right(), r.Bottom() ), g );
      return IntRect( p1, p2 );
    }

    IntPoint ImageToView_tr( const Point& p, const Geometry& g ){
      const faint::coord zoom = g.zoom.GetScaleFactor();
      float x2 = p.x * zoom - g.x0 + g.left_border;
      float y2 = p.y * zoom - g.y0 + g.top_border;
      return IntPoint( x2, y2 );
    }

    IntRect ImageToView_tr( const Rect& r, const Geometry& g ){
      IntPoint p1 = ImageToView_tr( Point( r.x, r.y ), g );
      IntPoint p2 = ImageToView_tr( Point( r.Right(), r.Bottom() ), g );
      return IntRect( p1, p2 );
    }
  }
}
