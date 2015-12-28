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
#include "objects/object.hh"
#include "geo/points.hh"

extern const std::string s_TypePath;

class ObjPath : public Object{
public:
  ObjPath( const Points&, const Settings& );
  Object* Clone() const override;
  void Draw( FaintDC& ) override;
  void DrawMask( FaintDC& ) override;
  std::vector<Point> GetAttachPoints() const override;
  IntRect GetRefreshRect() override;

  // Non-virtual
  std::vector<PathPt> GetPathPoints() const;
private:
  ObjPath( const ObjPath& ); // For clone
  Points m_points;
};

#endif
