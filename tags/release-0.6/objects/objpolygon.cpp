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

#include <cassert>
#include "objpolygon.hh"
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

const std::string s_TypePolygon = "Polygon";

ObjPolygon::ObjPolygon( const Points& points, const Settings& settings )
  : Object( &s_TypePolygon, points.GetTri(), with_point_editing(settings) ),
    m_points( points )
{}

ObjPolygon::ObjPolygon( const ObjPolygon& other )
  : Object( &s_TypePolygon, other.GetTri(), other.m_settings ),
    m_points( other.m_points )
{}

void ObjPolygon::Draw( FaintDC& dc ){
  dc.Polygon( m_points.GetPointsDumb( GetTri() ), m_settings );
}

void ObjPolygon::DrawMask( FaintDC& dc ) {
  dc.Polygon(m_points.GetPointsDumb( GetTri() ), mask_settings_fill(m_settings));
}

IntRect ObjPolygon::GetRefreshRect(){
  return truncated( inflated(GetRect(), m_settings.Get(ts_LineWidth) * 2 ) );
}

std::vector<Point> ObjPolygon::GetExtensionPoints() const{
  std::vector<Point> points = Vertices();
  if ( points.empty() ){
    return std::vector<Point>();
  }

  // Need to include the first point at the end, to get the midpoint on
  // the closing segment
  points.push_back(points[0]);
  return mid_points(points);
}

std::vector<Point> ObjPolygon::GetAttachPoints() const{
  return Vertices();
}

std::vector<Point> ObjPolygon::GetMovablePoints() const{
  return Vertices();
}

Object* ObjPolygon::Clone() const{
  return new ObjPolygon( *this );
}

Point ObjPolygon::GetPoint( size_t index ) const{
  assert( index < m_points.Size() );
  return Vertices()[index];
}

void ObjPolygon::SetPoint( const Point& pt, size_t index ){
  m_points.SetPoint(GetTri(), pt, index);
  SetTri(m_points.GetTri()); // Fixme
}

void ObjPolygon::InsertPoint( const Point& pt, size_t index ){
  m_points.InsertPoint(GetTri(), pt, index );
  SetTri(m_points.GetTri());
}

void ObjPolygon::RemovePoint( size_t index ){
  m_points.RemovePoint(GetTri(), index);
  SetTri(m_points.GetTri());
}

size_t ObjPolygon::NumPoints() const{
  return m_points.Size();
}

bool ObjPolygon::CyclicPoints() const{
  return true;
}

const std::vector<Point> ObjPolygon::Vertices() const{
  return m_points.GetPointsDumb( GetTri() );
}

bool ObjPolygon::Extendable() const{
  return true;
}

bool ObjPolygon::CanRemovePoint() const{
  return m_points.Size() > 3;
}
