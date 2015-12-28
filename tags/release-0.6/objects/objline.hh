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
  ObjLine( const ObjLine& );
  bool CanRemovePoint() const;
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  bool Extendable() const;
  std::vector<Point> GetAttachPoints() const;
  std::vector<Point> GetExtensionPoints() const;
  std::vector<Point> GetMovablePoints() const;
  Point GetPoint( size_t index ) const;
  IntRect GetRefreshRect();
  bool HitTest( const Point& );
  void InsertPoint(const Point&, size_t);
  size_t NumPoints() const;
  void RemovePoint(size_t);
  void SetPoint( const Point&, size_t index );
  std::string StatusString() const;

  // Non-virtual
  Point GetStart() const;
  Point GetEnd() const;
private:
  Points m_points;
  Point m_p0;
  Point m_p1;
  Rect GetLineRect() const;
  Rect GetArrowHeadRect() const;
  size_t m_lastIndex;
};

#endif
