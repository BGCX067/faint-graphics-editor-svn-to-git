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
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/settingid.hh"
#include "util/util.hh"

const std::string s_TypeTri = "TriObject";

Settings TriTextSettings(){
  Settings s;
  s.Set( ts_FgCol, faint::DrawSource(faint::Color(0,0,0)) );
  s.Set( ts_BgCol, faint::DrawSource(faint::Color( 255, 255, 255 ) ));
  s.Set( ts_SwapColors, false );
  s.Set( ts_FontSize, get_default_font_size() );
  s.Set( ts_FontFace, get_default_font_name() );
  s.Set( ts_FontBold, false );
  s.Set( ts_FontItalic, false );
  return s;
}

ObjTri::ObjTri( const Tri& tri, const Settings& s )
  : Object(&s_TypeTri, tri, s)
{}

void ObjTri::Draw( FaintDC& dc ){
  Tri tri(GetTri());
  Point p0(tri.P0());
  Point p1(tri.P1());
  Point p2(tri.P2());
  Point p3(tri.P3());
  dc.PolyLine( tri, vector_of(p0, p1), m_settings );
  dc.PolyLine( tri, vector_of(p0, p2), m_settings );
  Settings txtSettings(TriTextSettings());
  dc.Text( tri_from_rect(Rect( p0, Size(100,20) )), faint::utf8_string("p0="), txtSettings );
  dc.Text( tri_from_rect(Rect( p1, Size(100,20) )), faint::utf8_string("p1="), txtSettings );
  dc.Text( tri_from_rect(Rect( p2, Size(100,20) )), faint::utf8_string("p2="), txtSettings );
  dc.Text( tri_from_rect(Rect( p3, Size(100,20) )), faint::utf8_string("p3="), txtSettings );

  Rect r(bounding_rect(tri));
  dc.Text( tri_from_rect( Rect(r.TopRight() + Point(10,0), Size(100,100)) ), faint::utf8_string("w="), txtSettings );
}

void ObjTri::DrawMask( FaintDC& ){
}

IntRect ObjTri::GetRefreshRect(){
  return floored( inflated(GetRect(), 200) );
}

std::vector<Point> ObjTri::GetAttachPoints() const{
  return get_attach_points( GetTri() );
}

Object* ObjTri::Clone() const{
  return new ObjTri( GetTri(), GetSettings() );
}
