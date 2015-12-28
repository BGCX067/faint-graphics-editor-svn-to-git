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
#include <iterator>
#include "objects/objcomposite.hh"
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/util.hh"

const std::string s_TypeComposite = "Group";

ObjComposite::ObjComposite( const objects_t& objects, Ownership ownership )
  : Object(&s_TypeComposite, Tri()),
    m_objects(objects),
    m_ownership(ownership)
{
  assert(!objects.empty());
  Tri t = objects.front()->GetTri();
  Point minPt = min_coords( t.P0(), t.P1(), t.P2(), t.P3() );
  Point maxPt = max_coords( t.P0(), t.P1(), t.P2(), t.P3() );
  for ( objects_t::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it ){
    t = (*it)->GetTri();
    m_objTris.push_back(t);
    minPt = min_coords(minPt, min_coords( t.P0(), t.P1(), t.P2(), t.P3() ) );
    maxPt = max_coords(maxPt, max_coords( t.P0(), t.P1(), t.P2(), t.P3() ) );
  }

  SetTri(tri_from_rect(Rect(minPt, maxPt)));
  m_origTri = GetTri();
  for ( size_t i = 0; i != m_objTris.size(); i++ ){
    Tri objTri = m_objTris[i];
    m_objTris[i] = Tri( objTri.P0() - m_origTri.P0(), objTri.P1() - m_origTri.P0(), objTri.P2() - m_origTri.P0() );
  }
}

ObjComposite::ObjComposite( const ObjComposite& other )
  : Object(&s_TypeComposite, other.GetTri() ),
    m_origTri( other.m_origTri ),
    m_objTris( other.m_objTris ),
    m_ownership(OWNER)
{
  for ( size_t i = 0; i != other.m_objects.size(); i++ ){
    m_objects.push_back( other.m_objects[i]->Clone() );
  }
}

ObjComposite::~ObjComposite(){
  if ( m_ownership == OWNER ){
    for ( size_t i = 0; i != m_objects.size(); i++ ){
      delete m_objects[i];
    }
  }
}

void ObjComposite::Draw( FaintDC& dc ){
  Tri tri(GetTri());
  Adj a = get_adj( m_origTri, tri );

  faint::coord ht = fabs(m_origTri.P2().y - m_origTri.P1().y);
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Tri temp = m_objTris[i];

    Point p0( temp.P0() );
    Point p1( temp.P1() );
    Point p2( temp.P2() );

    p0.x += a.skew * ( fabs( ht - p0.y) / ht ) / a.scale_x;
    p1.x += a.skew * ( fabs(ht - p1.y ) / ht ) / a.scale_x;
    p2.x += a.skew * ( fabs( ht - p2.y ) / ht ) / a.scale_x;
    temp = translated( Tri( p0, p1, p2 ), m_origTri.P0().x + a.tr_x, m_origTri.P0().y + a.tr_y );
    temp = scaled( temp, Scale(a.scale_x, a.scale_y), tri.P3() );
    temp = rotated( temp, tri.Angle(), tri.P3() );
    m_objects[i]->SetTri( temp );
    m_objects[i]->Draw( dc );
  }
}

void ObjComposite::DrawMask( FaintDC& dc ){
  for ( size_t i = 0; i != m_objects.size(); i++ ){
    m_objects[i]->DrawMask( dc );
  }
}

IntRect ObjComposite::GetRefreshRect(){
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
  for ( objects_t::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it ){
    std::vector<Point> current( (*it)->GetAttachPoints() );
    std::copy( current.begin(), current.end(), std::back_inserter( points ) );
  }
  return points;
}
