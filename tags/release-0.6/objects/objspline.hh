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

#ifndef FAINT_OBJSPLINE_HH
#define FAINT_OBJSPLINE_HH
#include "object.hh"
#include "geo/points.hh"
extern const std::string s_TypeSpline;

Points spline_to_svg_path( const std::vector<Point>& );

class ObjSpline : public Object{
public:
  ObjSpline( const Points&, const Settings& );
  ObjSpline( const ObjSpline& );
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  std::vector<Point> GetAttachPoints() const;
  IntRect GetRefreshRect();
  std::vector<Point> GetSplinePoints() const; // Non-virtual
  bool ShowSizeBox() const;
private:
  Points m_points;
};

#endif
