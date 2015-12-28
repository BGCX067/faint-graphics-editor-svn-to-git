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

#ifndef FAINT_PATHPT_HH
#define FAINT_PATHPT_HH
#include "geo/geo-fwd.hh"
#include "geo/point.hh"
#include "geo/radii.hh"

namespace faint{

class PathPt {
public:
  enum class Type {
    ArcTo,
    Close,
    CubicBezier,
    LineTo,
    MoveTo
  };

  Type type;
  Point p;
  Point c;
  Point d;
  Radii r;
  coord axis_rotation;
  int large_arc_flag;
  int sweep_flag;

  static PathPt PathCloser(){
    return PathPt(Type::Close);
  }

  static PathPt MoveTo(const Point& p){
    return PathPt(Type::MoveTo, p);
  }

  static PathPt LineTo(const Point& p){
    return PathPt(Type::LineTo, p);
  }

  static PathPt CubicBezierTo(const Point& p, const Point& c, const Point& d){
    return PathPt(Type::CubicBezier, p, c, d);
  }

  static PathPt Arc(const Radii& r, coord in_axis_rotation, int in_large_arc_flag, int in_sweep_flag, const Point& p){
    PathPt pt(Type::ArcTo);
    pt.p = p;
    pt.c.x = 0.0;
    pt.c.y = 0.0;
    pt.d.x = 0.0;
    pt.d.y = 0.0;
    pt.r = r;
    pt.axis_rotation = in_axis_rotation;
    pt.large_arc_flag = in_large_arc_flag;
    pt.sweep_flag = in_sweep_flag;
    return pt;
  }

  bool ClosesPath() const{
    return type == Type::Close;
  }

  bool IsArc() const{
    return type == Type::ArcTo;
  }

  bool IsCubicBezier() const{
    return type == Type::CubicBezier;
  }

  bool IsLine() const{
    return type == Type::LineTo;
  }

  bool IsMove() const{
    return type == Type::MoveTo;
  }

  bool IsNotMove() const{
    return !IsMove();
  }

  PathPt Rotated(const Angle&, const Point& pivot) const;

private:
  PathPt(Type t)
    : type(t)
  {
    axis_rotation = 0;
    large_arc_flag = sweep_flag = 0;
  }

  PathPt(Type t, const Point& in_p)
    : type(t), p(in_p),
      axis_rotation(0), large_arc_flag(0), sweep_flag(0)
  {}

  PathPt(Type t, const Point& in_p, const Point& in_c, const Point& in_d) :
    type(t),
    p(in_p),
    c(in_c),
    d(in_d),
    axis_rotation(0),
    large_arc_flag(0),
    sweep_flag(0)
  {}
};

} // namespace
#endif
