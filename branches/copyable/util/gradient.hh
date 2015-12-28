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

#ifndef FAINT_GRADIENT_HH
#define FAINT_GRADIENT_HH
#include "geo/angle.hh"
#include "geo/point.hh"
#include "geo/radii.hh"
#include "util/color.hh"
#include "util/common-fwd.hh"
#include "util/optional.hh"

namespace faint{

class ColorStop{
public:
  ColorStop();
  ColorStop(const Color& c, double offset);
  Color GetColor() const;
  double GetOffset() const;
  bool operator<(const ColorStop&) const;
  bool operator==(const ColorStop&) const;
  bool operator!=(const ColorStop&) const;
private:
  Color m_color;
  double m_offset;
};

typedef std::vector<ColorStop> color_stops_t;

class LinearGradient{
public:
  LinearGradient();
  LinearGradient(const Angle&, const color_stops_t&);
  void Add(const ColorStop&);
  Angle GetAngle() const;
  index_t GetNumStops() const;
  bool GetObjectAligned() const;
  const ColorStop& GetStop(const index_t&) const;
  const color_stops_t& GetStops() const;
  void Remove(const index_t&);
  void SetAngle(const Angle&);
  void SetObjectAligned(bool);
  void SetStop(const index_t&, const ColorStop&);
  void SetStops(const color_stops_t&);
  bool operator==(const LinearGradient&) const;
  bool operator<(const LinearGradient&) const;
  bool operator>(const LinearGradient&) const;
private:
  Angle m_angle;
  bool m_objectAligned;
  color_stops_t m_stops;
};

LinearGradient unrotated(const LinearGradient&);
LinearGradient with_angle(const LinearGradient&, const Angle&);

class RadialGradient{
public:
  RadialGradient();
  RadialGradient(const Point&, const Radii&, const color_stops_t&);
  void Add(const ColorStop&);
  Point GetCenter() const;
  index_t GetNumStops() const;
  bool GetObjectAligned() const;
  Radii GetRadii() const;
  ColorStop GetStop(const index_t&) const;
  color_stops_t GetStops() const;
  void Remove(const index_t&);
  void SetCenter(const Point&);
  void SetObjectAligned(bool);
  void SetRadii(const Radii&);
  void SetStop(const index_t&, const ColorStop&);
  void SetStops(const color_stops_t&);
  bool operator==(const RadialGradient&) const;
  bool operator<(const RadialGradient&) const;
  bool operator>(const RadialGradient&) const;
private:
  Point m_center;
  bool m_objectAligned;
  Radii m_radii;
  color_stops_t m_stops;
};

class Gradient{
public:
  explicit Gradient(const LinearGradient&);
  explicit Gradient(const RadialGradient&);
  const LinearGradient& GetLinear() const;
  const RadialGradient& GetRadial() const;
  color_stops_t GetStops() const;
  bool IsLinear() const;
  bool IsRadial() const;
  bool operator==(const Gradient&) const;
private:
  Optional<LinearGradient> m_linear;
  Optional<RadialGradient> m_radial;
};

} // namespace

#endif
