// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_ROTATION_HH
#define FAINT_ROTATION_HH
#include "geo/geo-fwd.hh"
#include "geo/primitive.hh"

namespace faint{

class Rotation{
  // An unnormalized angle measure.
public:
  static Rotation Rad(radian);
  static Rotation Deg(degree);
  static Rotation FromAngle(const Angle&);
  static Rotation Zero();
  degree Deg() const;
  radian Rad() const;
  void operator+=(const Rotation&);
  bool operator==(const Rotation&) const;
  bool operator!=(const Rotation&) const;
  Rotation operator+(const Rotation&) const;
  Rotation operator/(coord) const;
  Rotation operator-() const;
  Rotation operator-(const Rotation&) const;
  bool operator<(const Rotation&) const;
  bool operator<=(const Rotation&) const;
  bool operator>(const Rotation&) const;
private:
  Rotation(radian);
  radian m_radians;
};

Rotation operator*(const Rotation&, coord);
Rotation operator*(coord, const Rotation&);

Rotation abs(const Rotation&);

coord sin(const Rotation&);
coord cos(const Rotation&);
coord tan(const Rotation&);
bool rather_zero(const Rotation&);

} // namespace

#endif
