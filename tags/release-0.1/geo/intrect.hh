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

#ifndef FAINT_INTRECT
#define FAINT_INTRECT

class IntRect{
public:
  IntRect();
  IntRect( const IntPoint&, const IntPoint& );
  IntRect( const IntPoint&, const IntSize& );
  IntSize GetSize() const;
  int Left() const;
  int Right() const;
  int Top() const;
  int Bottom() const;
  IntPoint TopLeft() const;
  IntPoint TopRight() const;
  IntPoint BottomLeft() const;
  IntPoint BottomRight() const;
  bool Contains( const IntPoint& ) const;
  void MoveTo( const IntPoint& );

  int x;
  int y;
  int w;
  int h;
};

bool Empty( const IntRect& );
IntRect Translated( const IntRect&, const IntPoint& );
IntRect Inflated( const IntRect&, int dx, int dy );
IntRect Inflated( const IntRect&, int d );
IntRect Union( const IntRect&, const IntRect& );
IntRect Intersection( const IntRect&, const IntRect& );

#endif
