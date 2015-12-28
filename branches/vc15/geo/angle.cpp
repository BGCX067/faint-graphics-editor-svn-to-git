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

#include <cassert>
#include <cmath>
#include "geo/angle.hh"
#include "util/math-constants.hh"

namespace faint{
const Angle pi = Angle::Rad(math::pi);
const degree degreePerRadian = 180.0 / pi.Rad();

static radian constrain_radians(radian radians){
  radians = std::fmod(radians, 2 * math::pi);
  return (radians < 0) ?
    radians + 2 * math::pi :
    radians;
}

Angle::Angle(radian radians)
  : m_radians(constrain_radians(radians))
{}

Angle Angle::Rad(radian a){
  return Angle(a);
}

Angle Angle::Deg(degree a){
  return Angle(a / degreePerRadian);
}

Angle Angle::Zero(){
  return Angle(0.0);
}

radian Angle::Rad() const{
  return m_radians;
}

degree Angle::Deg() const{
  return m_radians * degreePerRadian;
}

void Angle::operator+=(const Angle& other){
  m_radians = constrain_radians(m_radians + other.m_radians);
}

bool Angle::operator==(const Angle& other) const{
  return m_radians == other.m_radians;
}

bool Angle::operator!=(const Angle& other) const{
  return m_radians != other.m_radians;
}

Angle Angle::operator+(const Angle& other) const{
  return Angle(m_radians + other.m_radians);
}

Angle Angle::operator/(coord rhs) const{
  assert(rhs != 0);
  return Angle(m_radians / rhs);
}

Angle Angle::operator-() const{
  return Angle(-m_radians);
}

Angle Angle::operator-(const Angle& rhs) const{
  return Angle(m_radians - rhs.m_radians);
}

bool Angle::operator<(const Angle& rhs) const{
  return m_radians < rhs.m_radians;
}

bool Angle::operator<=(const Angle& rhs) const{
  return m_radians <= rhs.m_radians;
}

bool Angle::operator>(const Angle& rhs) const{
  return m_radians > rhs.m_radians;
}

Angle atan2(coord y, coord x){
  return Angle::Rad(std::atan2(y,x));
}

coord sin(const Angle& a){
  return std::sin(a.Rad());
}

coord cos(const Angle& a){
  return std::cos(a.Rad());
}

coord tan(const Angle& a){
  return std::tan(a.Rad());
}

Angle operator*(const Angle& lhs, coord rhs){
  return Angle::Rad(lhs.Rad() * rhs);
}

Angle operator*(coord lhs, const Angle& rhs){
  return Angle::Rad(lhs * rhs.Rad());
}

bool rather_zero(const Angle& angle){
  return fabs(angle.Rad()) <= coord_epsilon;
}

}
