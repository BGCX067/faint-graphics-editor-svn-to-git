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

#include "objrectangle.hh"
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/settingid.hh"
#include "util/settingutil.hh"

const std::string s_TypeRectangle = "Rectangle";

ObjRectangle::ObjRectangle( const Tri& tri, const Settings& s )
  : Object(&s_TypeRectangle, tri, s)
{}

Object* ObjRectangle::Clone() const{
  return new ObjRectangle( GetTri(), m_settings );
}

void ObjRectangle::Draw( FaintDC& dc ){
  dc.Rectangle( GetTri(), m_settings );
}

void ObjRectangle::DrawMask( FaintDC& dc ){
  dc.Rectangle(GetTri(), mask_settings_fill(m_settings));
}

std::vector<Point> ObjRectangle::GetAttachPoints() const{
  return get_attach_points( GetTri() );
}

IntRect ObjRectangle::GetRefreshRect(){
  return truncated( inflated( GetRect(), m_settings.Get(ts_LineWidth) ) );
}
