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

#include "line.hh"
#include "point.hh"

Line::Line() :
  m_p0(0,0),
  m_p1(0,0)
{}

Line::Line( const Point& p0, const Point& p1 )
  : m_p0(p0),
    m_p1(p1)
{}

Point Line::P0() const{
  return m_p0;
}

Point Line::P1() const{
  return m_p1;
}
