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

#ifndef FAINT_OBJRECTANGLE_HH
#define FAINT_OBJRECTANGLE_HH
#include "object.hh"
#include "geo/tri.hh"

FaintSettings GetRectangleSettings();
extern const std::string s_TypeRectangle;

class ObjRectangle : public Object{
public:
  ObjRectangle( const Tri&, const FaintSettings& );
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  std::vector<Point> GetAttachPoints();
  IntRect GetRefreshRect();
  std::vector<Point> GetResizePoints();
  bool HitTest( const Point& );
};

#endif

