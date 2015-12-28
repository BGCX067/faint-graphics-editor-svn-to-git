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

#ifndef FAINT_ANGLE_HH
#define FAINT_ANGLE_HH
#include "geo/primitive.hh"

namespace faint{
class Angle{
  // Normalized angle.
public:
  static Angle Rad(radian);
  static Angle Deg(degree);
  static Angle Zero();
  degree Deg() const;
  radian Rad() const;
  void operator+=(const Angle&);
  bool operator==(const Angle&) const;
  bool operator!=(const Angle&) const;
  Angle operator+(const Angle&) const;
  Angle operator/(coord) const;
  Angle operator-() const;
  Angle operator-(const Angle&) const;
  bool operator<(const Angle&) const;
  bool operator<=(const Angle&) const;
  bool operator>(const Angle&) const;
private:
  Angle(radian);
  radian m_radians;
};

Angle operator*(const Angle&, coord);
Angle operator*(coord, const Angle&);

// Signed angle between x-axis and x, y
Angle atan2(coord y, coord x);
coord sin(const Angle&);
coord cos(const Angle&);
coord tan(const Angle&);
bool rather_zero(const Angle&);

extern const Angle pi;
extern const degree degreePerRadian;

} // namespace

#endif
