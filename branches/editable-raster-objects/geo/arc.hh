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

#ifndef FAINT_ARC_HH
#define FAINT_ARC_HH
#include "geo/tri.hh"

class AngleSpan{
public:
  AngleSpan();
  AngleSpan( faint::radian start, faint::radian stop );
  faint::radian start;
  faint::radian stop;
};

std::vector<Point> arc_as_path( const Tri& tri, const AngleSpan& );

class ArcEndPoints{
public:
  ArcEndPoints( const Tri& tri, const AngleSpan& );
  Point operator[](size_t) const;
  operator std::vector<Point>() const;
  static const size_t num_points = 2;
private:
  Point m_p0;
  Point m_p1;
};

#endif
