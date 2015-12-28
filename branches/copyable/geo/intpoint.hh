// -*- coding: us-ascii-unix -*-
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

#ifndef FAINT_INTPOINT_HH
#define FAINT_INTPOINT_HH
#include "geo/primitive.hh"

namespace faint{

class IntPoint{
public:
  IntPoint();
  IntPoint(int x, int y);
  static IntPoint Both(int);
  void operator+=(const IntPoint&);
  void operator-=(const IntPoint&);
  bool operator==(const IntPoint&) const;
  bool operator!=(const IntPoint&) const;
  IntPoint operator-() const;
  bool operator<(const IntPoint&) const;
  int x;
  int y;

  IntPoint(coord, coord) = delete;
private:
  IntPoint(float, float); // Fixme: why not = delete?
  IntPoint(size_t, size_t); // Fixme: why not = delete?
};

bool fully_positive(const IntPoint&);
IntPoint max_coords(const IntPoint&, const IntPoint&);
IntPoint max_coords(const IntPoint&, const IntPoint&, const IntPoint&);
IntPoint min_coords(const IntPoint&, const IntPoint&);
IntPoint min_coords(const IntPoint&, const IntPoint&, const IntPoint&);
IntPoint operator-(const IntPoint&, const IntPoint&);
IntPoint operator-(const IntPoint&, int);
IntPoint operator+(const IntPoint&, int);
IntPoint operator+(const IntPoint&, const IntPoint&);
IntPoint operator*(const IntPoint&, const IntPoint&);
IntPoint operator/(const IntPoint&, const IntPoint&);
IntPoint operator/(const IntPoint&, int);
class Point;
Point operator*(const IntPoint&, coord);
Point operator*(coord scale, const IntPoint&);
Point operator/(const IntPoint&, coord);

}

#endif
