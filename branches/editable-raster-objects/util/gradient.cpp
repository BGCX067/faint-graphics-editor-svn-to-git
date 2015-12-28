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
#include "geo/geotypes.hh"
#include "util/gradient.hh"
namespace faint{

bool color_stops_less( const color_stops_t& lhs, const color_stops_t& rhs ){
  if ( lhs.size() < rhs.size() ){
    return true;
  }
  if ( lhs.size() > rhs.size() ){
    return false;
  }
  for ( size_t i = 0; i != lhs.size(); i++ ){
    if ( lhs[i] < rhs[i] ){
      return true;
    }
    else if ( lhs[i] != rhs[i] ){
      return false;
    }
  }
  return false;
}

ColorStop::ColorStop()
  : m_color(0,0,0),
    m_offset(0.0)
{}

ColorStop::ColorStop( const Color& c, double offset )
  : m_color(c),
    m_offset(offset)
{}

Color ColorStop::GetColor() const{
  return m_color;
}

double ColorStop::GetOffset() const{
  return m_offset;
}

bool ColorStop::operator<( const ColorStop& other ) const{
  if ( m_color < other.m_color ){
    return true;
  }
  else if ( m_color != other.m_color ){
    return false;
  }
  return m_offset < other.m_offset;
}

bool ColorStop::operator==( const ColorStop& other ) const{
  return m_color == other.m_color &&
    m_offset == other.m_offset; // Fixme: double comparison
}

bool ColorStop::operator!=( const ColorStop& other ) const{
  return m_color != other.m_color ||
    m_offset != other.m_offset; // Fixme: double comparison
}

LinearGradient::LinearGradient()
  : m_angle(0.0),
    m_objectAligned(true)
{}

LinearGradient::LinearGradient( faint::radian angle, const color_stops_t& stops )
  : m_angle(angle),
    m_objectAligned(true),
    m_stops(stops)
{}

void LinearGradient::Add( const ColorStop& stop ){
  m_stops.push_back(stop);
}

void LinearGradient::Remove( int index ){
  assert(index >= 0 );
  assert(static_cast<size_t>(index) < m_stops.size());
  m_stops.erase(m_stops.begin() + index);
}

faint::radian LinearGradient::GetAngle() const{
  return m_angle;
}

int LinearGradient::GetNumStops() const{
  return resigned(m_stops.size());
}

bool LinearGradient::GetObjectAligned() const{
  return m_objectAligned;
}

const ColorStop& LinearGradient::GetStop( int index ) const{
  assert(index >= 0 );
  assert(static_cast<size_t>(index) < m_stops.size() );
  return m_stops[index];
}

const color_stops_t& LinearGradient::GetStops() const{
  return m_stops;
}

void LinearGradient::SetAngle( faint::radian a ){
  m_angle = a;
}

void LinearGradient::SetObjectAligned( bool objectAligned ){
  m_objectAligned = objectAligned;
}

void LinearGradient::SetStop( int index, const ColorStop& stop ){
  assert(index >= 0);
  assert(static_cast<size_t>(index) < m_stops.size());
  m_stops[index] = stop;
}

void LinearGradient::SetStops( const color_stops_t& stops ){
  m_stops = stops;
}

bool LinearGradient::operator<( const LinearGradient& other ) const{
  if ( m_stops.size() < other.m_stops.size() ){
    return true;
  }
  else if ( m_stops.size() != other.m_stops.size() ){
    return false;
  }
  else if ( m_angle < other.m_angle ){
    return true;
  }
  else if ( m_angle != other.m_angle ){ // Fixme: double comparison
    return false;
  }
  else if ( m_objectAligned < other.m_objectAligned ){
    return true;
  }
  else if ( m_objectAligned != other.m_objectAligned ){
    return false;
  }
  return color_stops_less(m_stops, other.m_stops);
}

bool LinearGradient::operator==( const LinearGradient& other ) const{
  return
    m_angle == other.m_angle && // Fixme: double comparison
    m_objectAligned == other.m_objectAligned &&
    m_stops == other.m_stops;
}

LinearGradient unrotated( const faint::LinearGradient& g ){
  faint::LinearGradient g2(0.0, g.GetStops());
  g2.SetObjectAligned(g.GetObjectAligned());
  return g2;
}

RadialGradient::RadialGradient()
  : m_center(0,0),
    m_objectAligned(true),
    m_radii(0,0)
{}

RadialGradient::RadialGradient( const Point& center, const Radii& radii, const color_stops_t& stops )
  : m_center(center),
    m_objectAligned(true),
    m_radii(radii),
    m_stops(stops)
{}

void RadialGradient::Add( const ColorStop& stop ){
  m_stops.push_back(stop);
}

Point RadialGradient::GetCenter() const{
  return m_center;
}

int RadialGradient::GetNumStops() const{
  return resigned(m_stops.size());
}

bool RadialGradient::GetObjectAligned() const{
  return m_objectAligned;
}

Radii RadialGradient::GetRadii() const{
  return m_radii;
}

ColorStop RadialGradient::GetStop(int index) const{
  assert(static_cast<size_t>(index) < m_stops.size() );
  return m_stops[index];
}

color_stops_t RadialGradient::GetStops() const{
  return m_stops;
}

void RadialGradient::Remove( int index ){
  assert(index >= 0 );
  assert(static_cast<size_t>(index) < m_stops.size());
  m_stops.erase(m_stops.begin() + index);
}

void RadialGradient::SetCenter( const Point& center ){
  m_center = center;
}

void RadialGradient::SetObjectAligned( bool value ){
  m_objectAligned = value;
}

void RadialGradient::SetRadii( const Radii& radii ){
  m_radii = radii;
}

void RadialGradient::SetStop( int index, const ColorStop& stop ){
  assert(index >= 0);
  assert(static_cast<size_t>(index) < m_stops.size());
  m_stops[index] = stop;
}

void RadialGradient::SetStops( const color_stops_t& stops ){
  m_stops = stops;
}

bool RadialGradient::operator==( const RadialGradient& other ) const{
  return m_center == other.m_center &&
    m_radii == other.m_radii &&
    m_stops == other.m_stops;
}

template<typename T>
bool point_less( const T& lhs, const T& rhs ){
  return lhs.x < rhs.x || ( lhs.x == rhs.x && lhs.y < rhs.y );
}

bool RadialGradient::operator<( const RadialGradient& other ) const{
  if ( point_less(m_center, other.m_center ) ){
    return true;
  }
  else if ( m_center != other.m_center ){
    return false;
  }
  if ( point_less(m_radii, other.m_radii) ){
    return true;
  }
  else if ( m_radii != other.m_radii ){
    return false;
  }
  return color_stops_less(m_stops, other.m_stops);
}

Gradient::Gradient( const LinearGradient& linear )
  : m_linear(linear)
{}

Gradient::Gradient( const RadialGradient& radial )
  : m_radial( radial )
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

bool Gradient::operator==( const Gradient& other ) const{
  if ( m_linear.IsSet() ){
    return other.m_linear.IsSet() ?
      m_linear.Get() == other.m_linear.Get() :
      false;
  }
  else if ( m_radial.IsSet() ){
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
