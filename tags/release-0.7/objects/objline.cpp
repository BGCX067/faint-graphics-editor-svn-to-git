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
#include "objline.hh"
#include "rendering/faintdc.hh"
#include "util/settings.hh"
#include "util/angle.hh"
#include "util/settingid.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

const std::string s_TypeLine = "Line";
using faint::coord;

ObjLine::ObjLine( const Points& points, const Settings& settings )
  : Object( &s_TypeLine, points.GetTri(), with_point_editing(settings, points.Size() == 2)),
    m_points(points),
    m_lastIndex(0)
{
  assert(m_points.Size() >= 2);
  std::vector<Point> pts = m_points.GetPointsDumb();
  m_p0 = pts[0];
  m_p1 = pts[1];
}

ObjLine::ObjLine( const ObjLine& other )
  : Object( &s_TypeLine, other.GetTri(), other.m_settings ),
    m_points(other.m_points),
    m_p0(other.m_p0),
    m_p1(other.m_p1),
    m_lastIndex(0)
{}

void ObjLine::Draw( FaintDC& dc ){
  if ( m_settings.Get(ts_AntiAlias) || m_points.Size() > 2 ){
    dc.PolyLine( m_points.GetPointsDumb(GetTri()), m_settings );
  }
  else {
    // Fixme: This is a hack to avoid the Points-class adding rounding errors
    // to the "integer":y coordinates of raster lines, which could displace an end point
    // one pixel. See corresponding hack in LineTool::CreateCommand
    dc.Line(m_p0, m_p1, m_settings);
  }
}

void ObjLine::DrawMask( FaintDC& dc ){
  dc.PolyLine( m_points.GetPointsDumb(GetTri()), mask_settings_line(m_settings) );
}

Rect ObjLine::GetLineRect() const{
  faint::coord lineWidth(std::max( m_settings.Get( ts_LineWidth ), LITCRD(6.0) ));
  return inflated(GetRect(), lineWidth / LITCRD(2.0) );
}

Rect ObjLine::GetArrowHeadRect() const{
  Tri tri(GetTri());
  return get_arrowhead(Line(tri.P0(), tri.P1()), m_settings.Get( ts_LineWidth ) ).BoundingBox();
}

IntRect ObjLine::GetRefreshRect(){
  if ( m_settings.Get( ts_LineArrowHead ) == 1 ){
    return truncated(union_of(GetLineRect(), GetArrowHeadRect()));
  }
  return truncated(GetLineRect());
}

bool ObjLine::HitTest( const Point& pt ){
  if ( m_settings.Get( ts_LineArrowHead ) == 1 ){
    return GetLineRect().Contains(pt) || GetArrowHeadRect().Contains(pt);
  }
  return GetLineRect().Contains(pt);
}

std::vector<Point> ObjLine::GetMovablePoints() const{
  return m_points.GetPointsDumb(GetTri());
}

std::vector<Point> ObjLine::GetAttachPoints() const{
  std::vector<Point> pts(m_points.GetPointsDumb(GetTri()));
  std::vector<Point> extension(mid_points(pts));
  pts.insert(pts.end(), extension.begin(),extension.end() );
  return pts;
}

std::vector<Point> ObjLine::GetExtensionPoints() const{
  return mid_points(m_points.GetPointsDumb(GetTri()));
}

Object* ObjLine::Clone() const{
  return new ObjLine(*this);
}

Point ObjLine::GetPoint( size_t index ) const{
  assert(index < m_points.Size());
  return m_points.GetPointsDumb(GetTri())[index];
}

size_t ObjLine::NumPoints() const{
  return m_points.Size();
}

void ObjLine::SetPoint( const Point& pt, size_t index ){
  m_lastIndex = index;
  m_points.SetPoint(GetTri(), pt, index);
  SetTri(m_points.GetTri()); // Fixme
}

void ObjLine::InsertPoint( const Point& pt, size_t index ){
  m_points.InsertPoint(GetTri(), pt, index );
  SetTri(m_points.GetTri());
}

std::string ObjLine::StatusString() const{
  return str_line_status( m_lastIndex == 0 ? GetPoint(m_points.Size() - 1) : GetPoint( m_lastIndex - 1),
    GetPoint(m_lastIndex ) );

}

bool ObjLine::CanRemovePoint() const{
  return m_points.Size() > 2;
}

bool ObjLine::Extendable() const{
  return true;
}

void ObjLine::RemovePoint( size_t index ){
  m_points.RemovePoint(GetTri(), index);
  SetTri(m_points.GetTri());
}
