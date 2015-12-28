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

#include "objtri.hh"
#include "objtext.hh"
#include "faintdc.hh"
#include "tools/settingid.hh"
#include "util/objutil.hh"

const std::string s_TypeTri = "TriObject";

FaintSettings TriTextSettings(){
  FaintSettings s;
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_FontSize, 10 );
  s.Set( ts_FontFace, "Arial" ); // Fixme
  s.Set( ts_FontBold, false );
  s.Set( ts_FontItalic, false );
  s.Set( ts_TextAutoRect, false );
  return s;
}

ObjTri::ObjTri( const Tri& tri, const FaintSettings& s )
  : Object(&s_TypeTri, tri, s)
{}

void ObjTri::Draw( FaintDC& dc ){
  Tri tri(GetTri());
  Point p0(tri.P0());
  Point p1(tri.P1());
  Point p2(tri.P2());
  Point p3(tri.P3());
  dc.Line( p0.x, p0.y, p1.x, p1.y, m_settings );
  dc.Line( p0.x, p0.y, p2.x, p2.y, m_settings );
  FaintSettings txtSettings(TriTextSettings());
  dc.Text( TriFromRect( Rect( p0, Size(100,20) ) ), "p0=", txtSettings );
  dc.Text( TriFromRect( Rect( p1, Size(100,20) ) ), "p1=", txtSettings );
  dc.Text( TriFromRect( Rect( p2, Size(100,20) ) ), "p2=", txtSettings );
  dc.Text( TriFromRect( Rect( p3, Size(100,20) ) ), "p3=", txtSettings );

  Rect r(BoundingRect(tri));
  dc.Text( TriFromRect( Rect(r.TopRight() + Point(10,0), Size(100,100)) ), "w=", txtSettings );
}

void ObjTri::DrawMask( FaintDC& ){
}

IntRect ObjTri::GetRefreshRect(){
  return truncated( Inflated(GetRect(), 200) );
}

bool ObjTri::HitTest( const Point& p ){
  return GetRect().Contains(p);
}

std::vector<Point> ObjTri::GetResizePoints(){
  return Corners( GetRect() );
}

std::vector<Point> ObjTri::GetAttachPoints(){
  return AttachPointsFromTri( GetTri() );
}

Object* ObjTri::Clone() const{
  return new ObjTri( GetTri(), GetSettings() );
}
