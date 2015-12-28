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
  explicit Points( const std::vector<PathPt>& );
  explicit Points( const std::vector<Point>& );
  Points( const Points& );
  std::vector<PathPt> GetPoints( const Tri& ) const;
  std::vector<PathPt> GetPoints() const;
  std::vector<Point> GetPointsDumb() const;
  std::vector<Point> GetPointsDumb( const Tri& ) const;
  
  Tri GetTri() const;
  void SetTri( const Tri& );  
  
  void Append( const PathPt& );
  void Append( const Point& );
  void AdjustBack( const PathPt& );
  void AdjustBack( const Point& );
  void Clear();
  void PopBack();
  size_t Size() const;
private:
  Tri m_tri;
  std::vector<PathPt> m_points;

};

#endif
