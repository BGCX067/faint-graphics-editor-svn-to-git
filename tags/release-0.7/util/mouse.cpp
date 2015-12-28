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

#include "geo/geotypes.hh"
#include "geo/canvasgeo.hh"
#include "mouse.hh"

namespace faint {
  namespace mouse {
    Point view_to_image( const IntPoint& p, const Geometry& g ){
      const faint::coord zoom = g.zoom.GetScaleFactor();
      Point ptImg( (p.x + g.x0 - g.left_border ) / zoom,
        (p.y + g.y0 - g.top_border ) / zoom );
      return ptImg;
    }

    IntPoint image_to_view( const IntPoint& p, const Geometry& g ){
      const faint::coord zoom = g.zoom.GetScaleFactor();
      faint::coord x2 = p.x * zoom - g.x0 + g.left_border;
      faint::coord y2 = p.y * zoom - g.y0 + g.top_border;
      return truncated(Point(x2,y2));
    }

    IntPoint image_to_view_tr( const Point& p, const Geometry& g ){
      const faint::coord zoom = g.zoom.GetScaleFactor();
      faint::coord x2 = p.x * zoom - g.x0 + g.left_border;
      faint::coord y2 = p.y * zoom - g.y0 + g.top_border;
      return truncated(Point(x2,y2));
    }

    IntRect image_to_view_tr( const Rect& r, const Geometry& g ){
      IntPoint p1 = image_to_view_tr( Point( r.x, r.y ), g );
      IntPoint p2 = image_to_view_tr( Point( r.Right(), r.Bottom() ), g );
      return IntRect( p1, p2 );
    }

    IntRect image_to_view( const IntRect& r, const Geometry& g ){
      IntPoint p1 = image_to_view_tr( Point( r.x, r.y ), g );
      IntPoint p2 = image_to_view_tr( Point( r.Right(), r.Bottom() ), g );
      return IntRect( p1, p2 );
    }
  }
}
