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

extern const std::string s_TypeRectangle;

class ObjRectangle : public Object{
public:
  ObjRectangle( const Tri&, const Settings& );
  Object* Clone() const override;
  void Draw( FaintDC& ) override;
  void DrawMask( FaintDC& ) override;
  IntRect GetRefreshRect() override;
  std::vector<Point> GetAttachPoints() const override;
};

#endif
