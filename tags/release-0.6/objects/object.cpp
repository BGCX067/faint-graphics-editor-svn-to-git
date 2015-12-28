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
#include "object.hh"
#include "util/settingid.hh"

const std::string s_TypeObject = "Object";

const Settings NullObjectSettings;

const faint::Color mask_outside( 255,255,255 );
const faint::Color mask_fill( 0,255,0 );
const faint::Color mask_no_fill( 0,0,255 );
const faint::Color mask_edge( 255,0,0 );

Object::Object( const std::string* type, const Tri& tri, const Settings& s )
  : m_settings( s ),
    m_tri(tri)
{
  m_active = false;
  m_visible = true;
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

bool Object::Active() const{
  return m_active;
}

bool Object::CanRemovePoint() const{
  return false;
}

void Object::ClearActive(){
  m_active = false;
}

bool Object::CyclicPoints() const{
  return false;
}

bool Object::Extendable() const{
  return false;
}

std::vector<Point> Object::GetAttachPoints() const{
  return std::vector<Point>();
}

std::vector<Point> Object::GetExtensionPoints() const{
  return std::vector<Point>();
}

ObjectId Object::GetId() const{
  return m_id;
}

std::vector<Point> Object::GetMovablePoints() const {
  return std::vector<Point>();
}

Object* Object::GetObject( int ) {
  assert( false );
  return 0;
}

const Object* Object::GetObject( int ) const {
  assert( false );
  return 0;
}

size_t Object::GetObjectCount() const {
  return 0;
}

Point Object::GetPoint( size_t ) const{
  assert( false );
  return Point( 0, 0 );
}

Rect Object::GetRect() const{
  return bounding_rect(m_tri);
}

const Settings& Object::GetSettings() const{
  return m_settings;
}

Tri Object::GetTri() const{
  return m_tri;
}

std::vector<Point> Object::GetSnappingPoints() const{
  return GetAttachPoints();
}

const char* Object::GetType() const{
  return m_type.c_str();
}

bool Object::HitTest( const Point& p ){
  return GetRefreshRect().Contains(truncated(p)); // Fixme
}

bool Object::Inactive() const{
  return !m_active;
}

void Object::InsertPoint( const Point&, size_t ){
  assert( false );
}

size_t Object::NumPoints() const{
  return 0;
}

void Object::OnSetTri(){
}

void Object::RemovePoint( size_t ){
  assert( false );
}

void Object::SetActive(bool active){
  m_active = active;
}

void Object::SetPoint( const Point&, size_t ){
  assert( false );
}

void Object::SetTri( const Tri& t ){
  m_tri = t;
  OnSetTri();
}

void Object::SetVisible( bool visible ){
  m_visible = visible;
}

bool Object::ShowSizeBox() const{
  return false;
}

std::string Object::StatusString() const{
  return "";
}

bool Object::UpdateSettings( const Settings& s ){
  return m_settings.Update( s );
}

bool Object::Visible() const{
  return m_visible;
}
