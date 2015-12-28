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

#include "object.hh"
#include "tools/settingid.hh"

const std::string s_TypeObject = "Object";

const FaintSettings NullObjectSettings;

const faint::Color mask_outside( 255,255,255 );
const faint::Color mask_fill( 0,255,0 );
const faint::Color mask_edge( 255,0,0 );

Object::Object( const std::string* type, const Tri& tri, const FaintSettings& s )
  : m_settings( s ),
    m_tri(tri)
{
  m_active = false;
  m_type = *type;

  if ( m_settings.Has( ts_SwapColors ) && m_settings.Get( ts_SwapColors ) ){
    faint::Color fgCol = m_settings.Get( ts_BgCol );
    m_settings.Set( ts_BgCol, m_settings.Get( ts_FgCol ) );
    m_settings.Set( ts_FgCol, fgCol );
    m_settings.Set( ts_SwapColors, false );
  }
}

Object::~Object(){
}

Point Object::GetPoint( size_t /*index*/ ) const{
    assert( false );
    return Point( 0, 0 );
  }

const char* Object::GetType() const{
  return m_type.c_str();
}

bool Object::Active(){
  return m_active;
}

void Object::SetActive(){
  m_active = true;
}

void Object::SetPoint( const Point&, size_t ){
  assert( false );
}

void Object::SetTri( const Tri& t ){
  m_tri = t;
  OnSetTri();
}

std::string Object::StatusString() const{
  return "";
}

void Object::ClearActive(){
  m_active = false;
}

std::vector<Point> Object::GetAttachPoints(){
  return std::vector<Point>();
}

ObjectId Object::GetId() const{
  return m_id;
}

Rect Object::GetRect() const{
  return BoundingRect(m_tri);
}

const FaintSettings& Object::GetSettings() const{
  return m_settings;
}

Tri Object::GetTri() const{
  return m_tri;
}

const Object* Object::GetObject( int ) const {
  assert( false );
  return 0;
}

Object* Object::GetObject( int ) {
  assert( false );
  return 0;
}

size_t Object::GetObjectCount() const {
  return 0;
}

std::vector<Point> Object::GetMovablePoints() const {
  return std::vector<Point>();
}

bool Object::UpdateSettings( const FaintSettings& s ){
  return m_settings.Update( s );
}

Point Delta( Object* obj, const Point& p ){
  return p - obj->GetTri().P0();
}

void MoveTo( Object* obj, const Point& p ){
  OffsetBy( obj, Delta( obj, p ) );
}

void OffsetBy( Object* obj, const Point& d ){
  obj->SetTri( Translated( obj->GetTri(), d.x, d.y ) );
}

void OffsetBy( Object* obj, const IntPoint& d ){
  OffsetBy( obj, floated(d) );
}

void Object::OnSetTri(){
}
