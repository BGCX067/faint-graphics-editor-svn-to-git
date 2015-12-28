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

#ifndef FAINT_OBJLINE_HH
#define FAINT_OBJLINE_HH
#include "object.hh"
#include "geo/points.hh"
extern const std::string s_TypeLine;

class ObjLine : public Object{
public:
  ObjLine( const Points&, const Settings& );
  bool CanRemovePoint() const override;
  Object* Clone() const override;
  void Draw( FaintDC& ) override;
  void DrawMask( FaintDC& ) override;
  bool Extendable() const;
  std::vector<Point> GetAttachPoints() const override;
  std::vector<Point> GetExtensionPoints() const override;
  std::vector<Point> GetMovablePoints() const override;
  Point GetPoint( size_t index ) const override;
  IntRect GetRefreshRect() override;
  bool HitTest( const Point& ) override;
  void InsertPoint(const Point&, size_t) override;
  size_t NumPoints() const override;
  void RemovePoint(size_t) override;
  void SetPoint( const Point&, size_t index ) override;
  std::string StatusString() const override;
private:
  ObjLine( const ObjLine& ); // For Clone
  Rect GetLineRect() const;
  Rect GetArrowHeadRect() const;
  size_t m_lastIndex;
  Points m_points;
};

#endif
