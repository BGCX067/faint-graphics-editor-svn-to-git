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

#include "geo/arc.hh"
#include "objellipse.hh"
#include "rendering/faintdc.hh"
#include "util/angle.hh"
#include "util/objutil.hh"
#include "util/settingid.hh"
#include "util/settingutil.hh"

const std::string s_TypeEllipse = "Ellipse";

void draw_ellipse_span( FaintDC& dc, const Tri& tri, const AngleSpan& angles, const Settings& settings ){
  if ( angles.start == angles.stop ){
    dc.Ellipse( tri, settings );
  }
  else {
    dc.Arc( tri, angles, settings );
  }
}

ObjEllipse::ObjEllipse( const Tri& tri, const Settings& settings )
  : Object(&s_TypeEllipse, tri, with_point_editing(settings, start_enabled(false))),
    m_angleSpan(0.0, 0.0)
{}

Object* ObjEllipse::Clone() const{
  ObjEllipse* other = new ObjEllipse(GetTri(), m_settings);
  other->SetAngleSpan(m_angleSpan);
  return other;
}

void ObjEllipse::Draw( FaintDC& dc ){
  draw_ellipse_span( dc, GetTri(), m_angleSpan, m_settings );
}

void ObjEllipse::DrawMask( FaintDC& dc ){
  draw_ellipse_span(dc, GetTri(), m_angleSpan, mask_settings_fill(m_settings));
}

AngleSpan ObjEllipse::GetAngleSpan() const{
  return m_angleSpan;
}

IntRect ObjEllipse::GetRefreshRect(){
  const faint::coord width = m_settings.Get( ts_LineWidth );
  return floored( inflated(GetRect(), width ) );
}

std::vector<Point> ObjEllipse::GetAttachPoints() const{
  return get_attach_points( GetTri() );
}

std::vector<Point> ObjEllipse::GetMovablePoints() const{
  return ArcEndPoints( GetTri(), m_angleSpan );
}

size_t ObjEllipse::NumPoints() const{
  return ArcEndPoints::num_points;
}

Point ObjEllipse::GetPoint( size_t index ) const{
  return ArcEndPoints(GetTri(), m_angleSpan)[index];
}

void ObjEllipse::SetAngleSpan( const AngleSpan& span ){
  m_angleSpan = span;
}

void ObjEllipse::SetPoint( const Point& p, size_t index ){
  assert( index < 2 );
  Point c(center_point(GetTri()));
  if ( index == 0 ){
    m_angleSpan.start = rad_angle( c.x, c.y, p.x, p.y );
  }
  else{
    m_angleSpan.stop = rad_angle( c.x, c.y, p.x, p.y );
  }
}

void set_angle_span( ObjEllipse* ellipse, const AngleSpan& angleSpan ){
  ellipse->SetAngleSpan(angleSpan);
}
