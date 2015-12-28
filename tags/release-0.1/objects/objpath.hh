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

#ifndef FAINT_OBJPATH_HH
#define FAINT_OBJPATH_HH
#include "object.hh"
#include "faintdc.hh"
#include "geo/points.hh"

extern const std::string s_TypePath;

FaintSettings GetPathSettings();

class ObjPath : public Object{
public:
  ObjPath( const Points&, const FaintSettings& );
  ObjPath( const ObjPath& );
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  std::vector<Point> GetAttachPoints();
  Point GetPoint(int index);
  IntRect GetRefreshRect();
  std::vector<Point> GetResizePoints();
  bool HitTest( const Point& );

  // Non-virtual
  std::vector<PathPt> GetPathPoints() const;
private:
  Points m_points;
};

#endif
