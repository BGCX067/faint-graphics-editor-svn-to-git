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

#include "objellipse.hh"
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/settingid.hh"
#include "util/settingutil.hh"

const std::string s_TypeEllipse = "Ellipse";

ObjEllipse::ObjEllipse( const Tri& tri, const Settings& settings )
  : Object(&s_TypeEllipse, tri, settings)
{}

void ObjEllipse::Draw( FaintDC& dc ){
  dc.Ellipse( GetTri(), m_settings );
}

void ObjEllipse::DrawMask( FaintDC& dc ){
  dc.Ellipse(GetTri(), mask_settings_fill(m_settings));
}

IntRect ObjEllipse::GetRefreshRect(){
  const faint::coord width = m_settings.Get( ts_LineWidth );
  return truncated( inflated(GetRect(), width ) );
}

std::vector<Point> ObjEllipse::GetAttachPoints() const{
  return get_attach_points( GetTri() );
}

Object* ObjEllipse::Clone() const{
  return new ObjEllipse( GetTri(), m_settings );
}
