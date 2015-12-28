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

#ifndef FAINT_RECT
#define FAINT_RECT
class Point;
class Size;

class Rect{
public:
  Rect();
  Rect( const Point&, const Point& );
  Rect( const Point&, const Size& );
  Size GetSize() const;
  faint::coord Left() const;
  faint::coord Right() const;
  faint::coord Top() const;
  faint::coord Bottom() const;
  Point TopLeft() const;
  Point TopRight() const;
  Point BottomLeft() const;
  Point BottomRight() const;
  Point Center() const;
  bool Contains( const Point& ) const;

  faint::coord x;
  faint::coord y;
  faint::coord w;
  faint::coord h;
};

bool empty( const Rect& );
Rect inflated( const Rect&, faint::coord );
Rect inflated( const Rect&, faint::coord, faint::coord );
Rect intersection( const Rect&, const Rect& );
bool intersects( const Rect&, const Rect& );
Rect translated( const Rect&, const Point& );
Rect union_of( const Rect&, const Rect& );

#endif
