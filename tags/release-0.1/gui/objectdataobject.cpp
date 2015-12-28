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

#include "wx/dataobj.h"
#include "wx/dnd.h"
#include "objectdataobject.hh"
#include "object.hh"
#include "util/objutil.hh"

namespace faint{

ObjectDataObject::ObjectDataObject( const std::vector<Object*>& objects ){
  m_objects = Clone( objects );
}

ObjectDataObject::ObjectDataObject(){
}

ObjectDataObject::~ObjectDataObject(){
  DeleteObjects();
}

bool ObjectDataObject::IsSupported( const wxDataFormat& format, Direction ) const{
  if ( format == wxDataFormat("FaintObject" ) ){
    return true;
  }
  return false;
}

wxDataFormat ObjectDataObject::GetPreferredFormat( Direction ) const {
  return wxDataFormat("FaintObject");
}

void ObjectDataObject::GetAllFormats( wxDataFormat* formats, Direction) const{
  formats[0] = wxDataFormat("FaintObject");
}

size_t ObjectDataObject::GetFormatCount( Direction ) const{
  return 1;
}

size_t ObjectDataObject::GetDataSize( const wxDataFormat& f ) const{
  if ( f == wxDataFormat("FaintObject") ){
    return sizeof( Object* ) * m_objects.size();
  }
  return 0;
}

bool ObjectDataObject::GetDataHere( const wxDataFormat& format, void *buf ) const{
  if ( format == wxDataFormat("FaintObject") ){
    // Shallow copy - the deep copy is done in SetData, which is
    // called behind the scenes
    memcpy( buf, &(*m_objects.begin() ), sizeof(Object*) * m_objects.size() );
    return true;
  }
  return false;
}

bool ObjectDataObject::SetData( const wxDataFormat& format, size_t len, const void* buf ){
  if ( format != wxDataFormat("FaintObject") ){
    return false;
  }

  // Fixme: Need a proper serialized object format.
  DeleteObjects();
  Object** objects = new Object*[len / sizeof(Object*)];
  memcpy( objects, buf, len );
  for ( size_t i = 0; i !=  len / sizeof(Object*); i++){
    m_objects.push_back( objects[i]->Clone() );
  }
  delete[] objects;
  return true;
}

std::vector<Object*> ObjectDataObject::GetObjects() const{
  return Clone( m_objects );
}

void ObjectDataObject::DeleteObjects(){
  for ( size_t i = 0; i != m_objects.size(); i++ ){
    delete m_objects[i];
  }
  m_objects.clear();
}

} // Namespace faint
