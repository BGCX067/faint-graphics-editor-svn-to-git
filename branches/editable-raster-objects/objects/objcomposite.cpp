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

#include <algorithm>
#include <cassert>
#include <numeric>
#include "objects/objcomposite.hh"
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/settingid.hh"
#include "util/util.hh"
const std::string s_TypeComposite = "Group";

ObjComposite::ObjComposite( const objects_t& objects, Ownership ownership )
  : Object(&s_TypeComposite, Tri()),
    m_objects(objects),
    m_ownership(ownership)
{
  m_settings.Set(ts_AlignedResize, false);
  assert(!objects.empty());
  Tri t = objects.front()->GetTri();
  Point minPt = min_coords( t.P0(), t.P1(), t.P2(), t.P3() );
  Point maxPt = max_coords( t.P0(), t.P1(), t.P2(), t.P3() );
  for ( const Object* obj : m_objects ){
    t = obj->GetTri();
    m_objTris.push_back(t);
    minPt = min_coords(minPt, min_coords( t.P0(), t.P1(), t.P2(), t.P3() ) );
    maxPt = max_coords(maxPt, max_coords( t.P0(), t.P1(), t.P2(), t.P3() ) );
  }

  SetTri(tri_from_rect(Rect(minPt, maxPt)));
  m_origTri = GetTri();
  for ( Tri& objTri : m_objTris ){
    objTri = Tri( objTri.P0() - m_origTri.P0(), objTri.P1() - m_origTri.P0(), objTri.P2() - m_origTri.P0() );
  }
}

ObjComposite::ObjComposite( const ObjComposite& other )
  : Object(&s_TypeComposite, other.GetTri() ),
    m_origTri( other.m_origTri ),
    m_objTris( other.m_objTris ),
    m_ownership(Ownership::OWNER)
{
  m_settings = other.GetSettings();
  for ( const Object* obj : other.m_objects ){
    m_objects.push_back( obj->Clone() );
  }
}

ObjComposite::~ObjComposite(){
  if ( m_ownership == Ownership::OWNER ){
    for ( Object* obj : m_objects ){
      delete obj;
    }
  }
}

void ObjComposite::Draw( FaintDC& dc ){
  Tri tri(GetTri());
  Adj a = get_adj( m_origTri, tri );

  faint::coord ht = fabs(m_origTri.P2().y - m_origTri.P1().y);
  for ( size_t objNum = 0; objNum != m_objects.size(); objNum++ ){
    Tri temp = m_objTris[objNum];

    Point p0( temp.P0() );
    Point p1( temp.P1() );
    Point p2( temp.P2() );

    if ( a.scale_x != 0 && ht != 0 ){
      // Cannot skew objects scaled to nothing or with a height of zero.
      p0.x += a.skew * ( fabs( ht - p0.y) / ht ) / a.scale_x;
      p1.x += a.skew * ( fabs(ht - p1.y ) / ht ) / a.scale_x;
      p2.x += a.skew * ( fabs( ht - p2.y ) / ht ) / a.scale_x;
    }
    temp = translated( Tri( p0, p1, p2 ), m_origTri.P0().x + a.tr_x, m_origTri.P0().y + a.tr_y );
    temp = scaled( temp, Scale(a.scale_x, a.scale_y), tri.P3() );
    temp = rotated( temp, tri.Angle(), tri.P3() );
    m_objects[objNum]->SetTri( temp );
    m_objects[objNum]->Draw( dc );
  }
}

void ObjComposite::DrawMask( FaintDC& dc ){
  for ( Object* obj : m_objects ){
    obj->DrawMask( dc );
  }
}

IntRect ObjComposite::GetRefreshRect(){
  assert(!m_objects.empty());
  IntRect r( m_objects[0]->GetRefreshRect() );
  for ( size_t i = 1; i != m_objects.size(); i++ ){
    r = union_of( r, m_objects[i]->GetRefreshRect() );
  }
  return r;
}

Object* ObjComposite::Clone() const{
  return new ObjComposite( *this );
}

size_t ObjComposite::GetObjectCount() const{
  return m_objects.size();
}

Object* ObjComposite::GetObject( int index ){
  return m_objects[index];
}

const Object* ObjComposite::GetObject( int index ) const{
  return m_objects[index];
}

std::vector<Point> ObjComposite::GetSnappingPoints() const{
  return vector_of(GetRect().TopLeft());
}

// Fixme: This is probably horrendously slow for complex groups
std::vector<Point> ObjComposite::GetAttachPoints() const{
  std::vector<Point> points;
  for ( const Object* obj : m_objects ){
    std::vector<Point> current( obj->GetAttachPoints() );
    std::copy( current.begin(), current.end(), std::back_inserter( points ) );
  }
  return points;
}
