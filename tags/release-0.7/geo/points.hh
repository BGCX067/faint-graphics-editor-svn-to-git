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

#ifndef FAINT_POINTS_HH
#define FAINT_POINTS_HH
#include <vector>
#include "pathpt.hh"
#include "geotypes.hh"
#include "tri.hh"

class Points {
public:
  Points();
  Points( const Points& );
  explicit Points( const std::vector<PathPt>& );
  explicit Points( const std::vector<Point>& );
  void AdjustBack( const PathPt& );
  void AdjustBack( const Point& );
  void Append( const PathPt& );
  void Append( const Point& );
  void Clear();
  std::vector<PathPt> GetPoints( const Tri& ) const;
  std::vector<PathPt> GetPoints() const;
  std::vector<Point> GetPointsDumb() const;
  std::vector<Point> GetPointsDumb( const Tri& ) const;
  Tri GetTri() const;
  void InsertPoint( const Tri&, const Point&, size_t );
  PathPt PopBack();
  void RemovePoint( const Tri&, size_t );
  void SetPoint( const Tri&, const Point&, size_t index );
  void SetTri( const Tri& );
  size_t Size() const;
private:
  Tri m_tri;
  std::vector<PathPt> m_points;
  mutable std::vector<PathPt> m_cache;
  mutable Tri m_cacheTri;
};

#endif
