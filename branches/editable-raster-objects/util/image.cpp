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
#include "objects/object.hh"
#include "util/image.hh"
#include "util/objutil.hh"

namespace faint{

Image::Image( FrameProps& props )
  : m_delay(0),
    m_hasOriginal(false),
    m_objects( props.GetObjects() ),
    m_original( faint::get_null_bitmap() ),
    m_originalObjects(m_objects)
{
  if ( props.HasBitmap() ){
    m_bitmap = props.GetBitmap();
    m_delay = props.GetDelay();
    m_hotSpot = props.GetHotSpot();
  }
  else {
    m_bitmap = faint::Bitmap(props.GetSize(), props.GetBackgroundColor());
  }
  // "The object memory is now mine" - Image.
  props.Release();
}

Image::Image( const faint::Bitmap& bmp )
  : m_bitmap(bmp),
    m_delay(0),
    m_hasOriginal(false),
    m_original(get_null_bitmap())
{}

Image::Image( const Image& other )
  : m_bitmap(other.m_bitmap),
    m_delay(other.GetDelay()),
    m_hasOriginal(false),
    m_hotSpot(other.m_hotSpot),
    m_original(get_null_bitmap())
{
  m_originalObjects = m_objects = clone(other.GetObjects());
}

bool Image::HasStoredOriginal(){
  return m_hasOriginal;
}

void Image::StoreAsOriginal(){
  m_original = m_bitmap;
  m_hasOriginal = true;
}

void Image::SetBitmap( const Bitmap& bmp ){
  m_bitmap = bmp;
}

Image::~Image(){
  // Delete the objects the image was created with - any other objects
  // are deleted when the corresponding AddObject-commands are
  // deleted.
  for ( Object* obj : m_originalObjects ){
    delete obj;
  }
}

FrameId Image::GetId() const{
  return m_id;
}

IntSize Image::GetSize() const{
  return m_bitmap.GetSize();
}

void Image::Add( Object* object ){
  assert( !HasObject( object ) );
  m_objects.push_back( object );
}

void Image::Add( Object* object, size_t z ){
  assert( !HasObject( object ) );
  assert( z <= m_objects.size() );
  m_objects.insert( m_objects.begin() + z, object );
}

bool Image::Deselect( const Object* object ){
  return remove(object, from(m_objectSelection));
}

bool Image::Deselect( const objects_t& objects ){
  return remove(objects, from(m_objectSelection));
}

void Image::DeselectObjects(){
  m_objectSelection.clear();
}

RasterSelection& Image::FloatRasterSelection( const copy_selected& copy ){
  m_rasterSelection.BeginFloat(m_bitmap, copy);
  return m_rasterSelection;
}

const objects_t& Image::GetObjects() const{
  return m_objects;
}

const objects_t& Image::GetObjectSelection() const{
  return m_objectSelection;
}

size_t Image::GetObjectZ( const Object* obj ) const {
  for ( size_t i = 0; i != m_objects.size(); i++ ){
    if ( m_objects[i] == obj ){
      return i;
    }
  }
  assert( false );
  return 0;
}

RasterSelection& Image::GetRasterSelection(){
  return m_rasterSelection;
}

const RasterSelection& Image::GetRasterSelection() const{
  return m_rasterSelection;
}

void Image::SelectObjects( const objects_t& objects ){
  for ( Object* obj : objects ){
    if ( lacks(m_objectSelection, obj) ){
      size_t pos = get_sorted_insertion_pos( obj, m_objectSelection, m_objects );
      m_objectSelection.insert(m_objectSelection.begin() + pos, obj);
    }
  }
}

void Image::SetObjectZ( Object* obj, size_t z ){
  bool wasSelected = contains(m_objectSelection, obj);
  Remove( obj );
  z = std::min( z, m_objects.size() );
  m_objects.insert( m_objects.begin() + z, obj );
  if ( wasSelected ){
    size_t pos = get_sorted_insertion_pos(obj, m_objectSelection, m_objects);
    m_objectSelection.insert(m_objectSelection.begin() + pos, obj);
  }
}

void Image::Remove( Object* obj ){
  remove(obj, from(m_objectSelection));
  bool removed = remove(obj, from(m_objects));
  assert(removed);
}

size_t Image::GetNumObjects() const{
  return m_objects.size();
}

bool Image::Has( const ObjectId& objId ) const{
  for ( const Object* obj : m_objects ){
    if ( is_or_has(obj, objId ) ){
      return true;
    }
  }
  return false;
}

bool Image::HasObject( const Object* obj ) const {
  return contains(m_objects, obj);
}

void Image::Revert(){
  assert(m_hasOriginal);
  m_bitmap = m_original;
}

const Bitmap& Image::GetBitmap() const{
  return m_bitmap;
}

delay_t Image::GetDelay() const{
  return m_delay;
}

IntPoint Image::GetHotSpot() const{
  return m_hotSpot;
}

Bitmap& Image::GetBitmap(){
  return m_bitmap;
}

void Image::SetDelay( const delay_t& delay ){
  m_delay = delay;
}

void Image::SetHotSpot( const IntPoint& hotSpot ){
  m_hotSpot = hotSpot;
}

IntRect image_rect( const Image& image ){
  return IntRect(IntPoint(0,0), image.GetSize());
}

} // namespace faint
