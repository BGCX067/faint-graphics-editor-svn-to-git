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

#include "objects/objpath.hh"
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/settingid.hh"
#include "util/settings.hh"
#include "util/settingutil.hh"

const std::string s_TypePath = "Path";

ObjPath::ObjPath( const Points& points, const Settings& settings )
  : Object( &s_TypePath, points.GetTri(), settings ),
    m_points( points )
{}

ObjPath::ObjPath( const ObjPath& other ) :
  Object( &s_TypePath, other.GetTri(), other.m_settings ),
  m_points( other.m_points )
{}

void ObjPath::Draw( FaintDC& dc ){
  dc.Path( m_points.GetPoints(GetTri()), m_settings );
}

void ObjPath::DrawMask( FaintDC& dc ){
  // Fixme: A path isn't necessarily closed, so mask_settings_fill may need consideration
  dc.Path( m_points.GetPoints(GetTri()), mask_settings_fill(m_settings));
}

IntRect ObjPath::GetRefreshRect(){
  return floored(inflated(GetRect(), m_settings.Get(ts_LineWidth)));
}

std::vector<Point> ObjPath::GetAttachPoints() const{
  return std::vector<Point>();
}

Object* ObjPath::Clone() const{
  return new ObjPath( *this );
}

std::vector<PathPt> ObjPath::GetPathPoints() const{
  return m_points.GetPoints(GetTri());
}
