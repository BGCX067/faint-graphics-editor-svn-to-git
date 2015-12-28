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

#include <cmath>
#include "util/gradient.hh"
#include "util/index.hh"

namespace faint{

static bool color_stops_less(const color_stops_t& lhs, const color_stops_t& rhs ){
  if (lhs.size() < rhs.size() ){
    return true;
  }
  if (lhs.size() > rhs.size() ){
    return false;
  }
  for (size_t i = 0; i != lhs.size(); i++ ){
    if (lhs[i] < rhs[i] ){
      return true;
    }
    else if (lhs[i] != rhs[i] ){
      return false;
    }
  }
  return false;
}

template<typename T>
bool point_less(const T& lhs, const T& rhs ){
  return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y );
}

ColorStop::ColorStop()
  : m_color(0,0,0),
    m_offset(0.0)
{}

ColorStop::ColorStop(const Color& c, double offset )
  : m_color(c),
    m_offset(offset)
{}

Color ColorStop::GetColor() const{
  return m_color;
}

double ColorStop::GetOffset() const{
  return m_offset;
}

bool ColorStop::operator<(const ColorStop& other ) const{
  if (m_color < other.m_color ){
    return true;
  }
  else if (m_color != other.m_color ){
    return false;
  }
  return m_offset < other.m_offset;
}

bool ColorStop::operator==(const ColorStop& other ) const{
  return m_color == other.m_color &&
    m_offset == other.m_offset;
}

bool ColorStop::operator!=(const ColorStop& other ) const{
  return m_color != other.m_color ||
    m_offset != other.m_offset;
}

LinearGradient::LinearGradient()
  : m_angle(Angle::Zero()),
    m_objectAligned(true)
{}

LinearGradient::LinearGradient(const Angle& angle, const color_stops_t& stops )
  : m_angle(angle),
    m_objectAligned(true),
    m_stops(stops)
{}

void LinearGradient::Add(const ColorStop& stop ){
  m_stops.push_back(stop);
}

void LinearGradient::Remove(const index_t& index ){
  assert(valid_index(index, m_stops));
  m_stops.erase(begin(m_stops) + index.Get());
}

Angle LinearGradient::GetAngle() const{
  return m_angle;
}

index_t LinearGradient::GetNumStops() const{
  return index_t(resigned(m_stops.size()));
}

bool LinearGradient::GetObjectAligned() const{
  return m_objectAligned;
}

const ColorStop& LinearGradient::GetStop(const index_t& index ) const{
  assert(valid_index(index, m_stops));
  return m_stops[to_size_t(index)];
}

const color_stops_t& LinearGradient::GetStops() const{
  return m_stops;
}

void LinearGradient::SetAngle(const Angle& a ){
  m_angle = a;
}

void LinearGradient::SetObjectAligned(bool objectAligned ){
  m_objectAligned = objectAligned;
}

void LinearGradient::SetStop(const index_t& index, const ColorStop& stop ){
  assert(valid_index(index, m_stops));
  m_stops[to_size_t(index)] = stop;
}

void LinearGradient::SetStops(const color_stops_t& stops ){
  m_stops = stops;
}

bool LinearGradient::operator<(const LinearGradient& other ) const{
  if (m_stops.size() < other.m_stops.size() ){
    return true;
  }
  else if (m_stops.size() != other.m_stops.size() ){
    return false;
  }
  else if (m_angle < other.m_angle ){
    return true;
  }
  else if (m_angle != other.m_angle ){
    return false;
  }
  else if (m_objectAligned < other.m_objectAligned ){
    return true;
  }
  else if (m_objectAligned != other.m_objectAligned ){
    return false;
  }
  return color_stops_less(m_stops, other.m_stops);
}

bool LinearGradient::operator==(const LinearGradient& other ) const{
  return
    m_angle == other.m_angle &&
    m_objectAligned == other.m_objectAligned &&
    m_stops == other.m_stops;
}

bool LinearGradient::operator>(const LinearGradient& other) const{
  return !operator<(other) && !operator==(other); // Fixme: Slow

}
LinearGradient unrotated(const LinearGradient& g ){
  LinearGradient g2(Angle::Zero(), g.GetStops());
  g2.SetObjectAligned(g.GetObjectAligned());
  return g2;
}

LinearGradient with_angle(const LinearGradient& g, const Angle& angle ){
  LinearGradient g2(g);
  g2.SetAngle(angle);
  return g2;
}

RadialGradient::RadialGradient()
  : m_center(0,0),
    m_objectAligned(true),
    m_radii(0,0)
{}

RadialGradient::RadialGradient(const Point& center, const Radii& radii, const color_stops_t& stops )
  : m_center(center),
    m_objectAligned(true),
    m_radii(radii),
    m_stops(stops)
{}

void RadialGradient::Add(const ColorStop& stop ){
  m_stops.push_back(stop);
}

Point RadialGradient::GetCenter() const{
  return m_center;
}

index_t RadialGradient::GetNumStops() const{
  return index_t(resigned(m_stops.size()));
}

bool RadialGradient::GetObjectAligned() const{
  return m_objectAligned;
}

Radii RadialGradient::GetRadii() const{
  return m_radii;
}

ColorStop RadialGradient::GetStop(const index_t& index) const{
  assert(valid_index(index, m_stops));
  return m_stops[to_size_t(index)];
}

color_stops_t RadialGradient::GetStops() const{
  return m_stops;
}

void RadialGradient::Remove(const index_t& index){
  assert(valid_index(index, m_stops));
  m_stops.erase(begin(m_stops) + index.Get());
}

void RadialGradient::SetCenter(const Point& center ){
  m_center = center;
}

void RadialGradient::SetObjectAligned(bool value ){
  m_objectAligned = value;
}

void RadialGradient::SetRadii(const Radii& radii ){
  m_radii = radii;
}

void RadialGradient::SetStop(const index_t& index, const ColorStop& stop ){
  assert(valid_index(index, m_stops));
  m_stops[to_size_t(index)] = stop;
}

void RadialGradient::SetStops(const color_stops_t& stops ){
  m_stops = stops;
}

bool RadialGradient::operator==(const RadialGradient& other ) const{
  return m_center == other.m_center &&
    m_radii == other.m_radii &&
    m_stops == other.m_stops;
}

bool RadialGradient::operator<(const RadialGradient& other ) const{
  if (point_less(m_center, other.m_center ) ){
    return true;
  }
  else if (m_center != other.m_center ){
    return false;
  }
  if (point_less(m_radii, other.m_radii) ){
    return true;
  }
  else if (m_radii != other.m_radii ){
    return false;
  }
  return color_stops_less(m_stops, other.m_stops);
}

bool RadialGradient::operator>(const  RadialGradient& other ) const{
  return !operator<(other) && !operator==(other); // Fixme: Slow
}

Gradient::Gradient(const LinearGradient& linear )
  : m_linear(linear)
{}

Gradient::Gradient(const RadialGradient& radial )
  : m_radial(radial )
{}

color_stops_t Gradient::GetStops() const{
  return m_linear.IsSet() ?
    m_linear.Get().GetStops() :
    m_radial.Get().GetStops();
}

bool Gradient::IsLinear() const{
  return m_linear.IsSet();
}

bool Gradient::IsRadial() const{
  return m_radial.IsSet();
}

bool Gradient::operator==(const Gradient& other ) const{
  if (m_linear.IsSet() ){
    return other.m_linear.IsSet() ?
      m_linear.Get() == other.m_linear.Get() :
      false;
  }
  else if (m_radial.IsSet() ){
    return other.m_radial.IsSet() ?
      m_radial.Get() == other.m_radial.Get() :
      false;
  }

  assert(false);
  return false;
}

const LinearGradient& Gradient::GetLinear() const{
  return m_linear.Get();
}

const RadialGradient& Gradient::GetRadial() const{
  return m_radial.Get();
}

} // namespace
