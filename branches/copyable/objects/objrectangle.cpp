// -*- coding: us-ascii-unix -*-
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

#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/pathpt.hh"
#include "geo/tri.hh"
#include "objects/objrectangle.hh"
#include "rendering/faint-dc.hh"
#include "text/utf8-string.hh"
#include "util/object-util.hh"
#include "util/setting-id.hh"
#include "util/setting-util.hh"

namespace faint{

class ObjRectangle : public Object{
public:
  ObjRectangle( const Tri& tri, const Settings& s)
    : Object(s),
      m_tri(tri)
  {}

  Object* Clone() const override{
    return new ObjRectangle( m_tri, m_settings );
  }

  void Draw( FaintDC& dc ) override{
    dc.Rectangle( m_tri, m_settings );
  }

  void DrawMask( FaintDC& dc ) override{
    dc.Rectangle(m_tri, mask_settings_fill(m_settings));
  }

  std::vector<Point> GetAttachPoints() const override{
    return get_attach_points( m_tri );
  }

  std::vector<PathPt> GetPath() const override{
    return {
      PathPt::MoveTo(m_tri.P0()),
      PathPt::LineTo(m_tri.P1()),
      PathPt::LineTo(m_tri.P3()),
      PathPt::LineTo(m_tri.P2()),
      PathPt::PathCloser()
    };
  }

  IntRect GetRefreshRect() const override{
    return floored(bounding_rect(m_tri, m_settings));
  }

  Tri GetTri() const override{
    return m_tri;
  }

  faint::utf8_string GetType() const override{
    return "Rectangle";
  }

  void SetTri(const Tri& t) override{
    m_tri = t;
  }
private:
  Tri m_tri;
};

Object* create_rectangle_object(const Tri& tri, const Settings& s){
  return new ObjRectangle(tri, s);
}

} // namespace
