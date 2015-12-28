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
#include "geo/rotation.hh"

namespace faint{

Rotation::Rotation(radian radians)
  : m_radians(radians)
{}

Rotation Rotation::Rad(radian a){
  return Rotation(a);
}

Rotation Rotation::FromAngle(const Angle& a){
  return Rotation(a.Rad());
}

Rotation Rotation::Deg(degree a){
  return Rotation(a / degreePerRadian);
}

Rotation Rotation::Zero(){
  return Rotation(0.0);
}

radian Rotation::Rad() const{
  return m_radians;
}

degree Rotation::Deg() const{
  return m_radians * degreePerRadian;
}

void Rotation::operator+=(const Rotation& other){
  m_radians += other.m_radians;
}

bool Rotation::operator==(const Rotation& other) const{
  return m_radians == other.m_radians;
}

bool Rotation::operator!=(const Rotation& other) const{
  return m_radians != other.m_radians;
}

Rotation Rotation::operator+(const Rotation& other) const{
  return Rotation(m_radians + other.m_radians);
}

Rotation Rotation::operator/(coord rhs) const{
  assert(rhs != 0);
  return Rotation(m_radians / rhs);
}

Rotation Rotation::operator-() const{
  return Rotation(-m_radians);
}

Rotation Rotation::operator-(const Rotation& rhs) const{
  return Rotation(m_radians - rhs.m_radians);
}

bool Rotation::operator<(const Rotation& rhs) const{
  return m_radians < rhs.m_radians;
}

bool Rotation::operator<=(const Rotation& rhs) const{
  return m_radians <= rhs.m_radians;
}

bool Rotation::operator>(const Rotation& rhs) const{
  return m_radians > rhs.m_radians;
}

Rotation abs(const Rotation& a){
  return Rotation::Rad(std::fabs(a.Rad()));
}

coord sin(const Rotation& a){
  return std::sin(a.Rad());
}

coord cos(const Rotation& a){
  return std::cos(a.Rad());
}

coord tan(const Rotation& a){
  return std::tan(a.Rad());
}

Rotation operator*(const Rotation& lhs, coord rhs){
  return Rotation::Rad(lhs.Rad() * rhs);
}

Rotation operator*(coord lhs, const Rotation& rhs){
  return Rotation::Rad(lhs * rhs.Rad());
}

bool rather_zero(const Rotation& rot){
  return fabs(rot.Rad()) <= coord_epsilon;
}

}
