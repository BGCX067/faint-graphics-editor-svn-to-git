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

#ifndef FAINT_OBJPOLYGON_HH
#define FAINT_OBJPOLYGON_HH
#include "object.hh"
#include "geo/points.hh"

extern const std::string s_TypePolygon;

class ObjPolygon : public Object{
public:
  ObjPolygon( const Points&, const Settings& );
  ObjPolygon( const ObjPolygon& );
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  std::vector<Point> GetAttachPoints() const;
  std::vector<Point> GetMovablePoints() const;
  std::vector<Point> GetExtensionPoints() const;
  IntRect GetRefreshRect();
  bool Extendable() const;
  bool CanRemovePoint() const;

  Point GetPoint(size_t) const;
  void SetPoint(const Point&, size_t);
  void InsertPoint(const Point&, size_t);
  void RemovePoint(size_t);

  size_t NumPoints() const;
  bool CyclicPoints() const;

  // Non-virtual
  const std::vector<Point> Vertices() const;
private:
  Points m_points;
};

#endif
