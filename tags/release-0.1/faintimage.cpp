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
#include <cstring>
#include "bitmap/cairo_util.h"
#include "bitmap/bitmap.h"
#include "faintimage.hh"
#include "faintdc.hh"
#include "objects/object.hh"

ImageProps::ImageProps()
  : m_ok(true),
    m_hasBitmap(false),
    m_size(640,480)
{}

ImageProps::ImageProps(const faint::Bitmap& bmp )
  : m_ok(true),
    m_hasBitmap(true),
    m_bitmap(bmp),
    m_size(640,480),
    m_owner(true)
{}

ImageProps::ImageProps( const faint::Bitmap& bmp, const std::vector<Object*>& objects )
  : m_ok(true),
    m_hasBitmap(true),
    m_bitmap(bmp),
    m_objects(objects),
    m_size(640,480),
    m_owner(true)
{}

ImageProps::~ImageProps(){
  if ( m_owner ){
    for ( size_t i = 0; i != m_objects.size(); i++ ){
      // Not deleting from m_allObjects, since the ObjComposites
      // are responsible for deleting sub-objects.
      delete m_objects[i];
    }
    m_objects.clear();
    m_allObjects.clear();
  }
}

ImageProps::ObjId ImageProps::AddObject(Object* obj){
  m_objects.push_back(obj);
  m_allObjects.push_back(obj);
  return static_cast<ObjId>(m_allObjects.size()) - 1;
}

faint::Bitmap ImageProps::GetBitmap() const{
  assert(m_hasBitmap);
  assert(m_ok);
  return m_bitmap;
}

std::string ImageProps::GetError() const{
  assert(!m_ok);
  return m_error;
}

std::vector<Object*> ImageProps::GetObjects() const{
  return m_objects;
}

Object* ImageProps::GetObject(ImageProps::ObjId id){
  return m_allObjects[id];
}

IntSize ImageProps::GetSize() const{
  return m_hasBitmap ? m_bitmap.Size() : m_size;

}

bool ImageProps::HasBitmap() const{
  return m_hasBitmap;
}

bool ImageProps::HasObject( ImageProps::ObjId id ) const{
  return 0 <= id  && id < m_allObjects.size();
}

bool ImageProps::IsOk() const{
  return m_ok;
}

bool ImageProps::IsTopLevel( ObjId id ) const{
  assert( HasObject(id) );
  Object* obj = m_allObjects.at(id);
  std::vector<Object*>::const_iterator iter = std::find(m_objects.begin(), m_objects.end(), obj );
  return iter != m_objects.end();
}

void ImageProps::Release(){
  m_owner = false;
}

void ImageProps::RemoveObject( Object* obj ){
  std::vector<Object*>::iterator iter = std::find( m_objects.begin(), m_objects.end(), obj );
  assert( iter != m_objects.end() );
  m_objects.erase(iter);
}

void ImageProps::SetBitmap( const faint::Bitmap& bmp ){
  m_bitmap = bmp;
  m_hasBitmap = true;
}

void ImageProps::SetSize( const IntSize& size ){
  m_size = size;
}

void ImageProps::SetError( const std::string& error ){
  m_error = error;
  m_ok = false;
}


namespace faint{

Image::Image( ImageProps& props )
  : m_objects( props.GetObjects() ),
    m_originalObjects(m_objects)
{
  if ( props.HasBitmap() ){
    m_bitmap = props.GetBitmap();
  }
  else {
    m_bitmap = CairoCompatibleBitmap( props.GetSize() );
    // Clear with white (todo: Make bg color dependent on props)
    memset(m_bitmap.m_data, 255, m_bitmap.m_h * m_bitmap.m_row_stride );    
  }
  m_original = m_bitmap;
  // "The object memory is now mine" - Image.
  props.Release();
}

Image::Image( const IntSize& size, const std::vector<Object*>& objects )
  : m_bitmap( faint::CairoCompatibleBitmap( size.w, size.h ) ),
    m_original( faint::CairoCompatibleBitmap( size.w, size.h ) )
{
  memset(m_bitmap.m_data, 255, m_bitmap.m_h * m_bitmap.m_row_stride );
  memset(m_original.m_data, 255, m_bitmap.m_h * m_bitmap.m_row_stride );
  m_objects = objects;
}

Image::Image( const faint::Bitmap& bmp )
  : m_bitmap( bmp ),
    m_original( bmp )
{}

Image::Image( const faint::Bitmap& bmp, const std::vector<Object*>& objects )
  : m_objects(objects),
    m_originalObjects(objects),
    m_bitmap(bmp),
    m_original(bmp)
{}

void Image::SetBitmap( const Bitmap& bmp ){
  m_bitmap = bmp;
}

void Image::SetBaseBitmap( const Bitmap& bmp ){
  m_bitmap = bmp;
  m_original = bmp;
}

Image::~Image(){
  // Delete the objects the image was created with - any other objects
  // are deleted when the corresponding AddObject-commands are
  // deleted.
  for ( size_t i = 0; i != m_originalObjects.size(); i++ ){
    delete m_originalObjects[i];
  }
}

IntSize Image::GetSize() const{
  return IntSize( m_bitmap.m_w, m_bitmap.m_h );
}

void Image::AddObject( Object* object ){
  assert( !HasObject( object ) );
  m_objects.push_back( object );
}

void Image::AddObject( Object* object, size_t z ){
  assert( !HasObject( object ) );
  if ( z == m_objects.size() ){
    m_objects.push_back( object );
  }
  else {
    m_objects.insert( m_objects.begin() + z, object );
  }
}

std::vector<Object*> Image::GetObjects() const{
  return m_objects;
}

int Image::GetObjectZ( Object* obj ) const {
  for ( size_t i = 0; i != m_objects.size(); i++ ){
    if ( m_objects[i] == obj ){
      return i;
    }
  }
  return -1;
}
void Image::SetObjectZ( Object* obj, size_t z ){
  int oldPos = GetObjectZ(obj);
  assert( oldPos != -1 );
  RemoveObject( obj );
  z = std::min( z, m_objects.size() );
  m_objects.insert( m_objects.begin() + z, obj );
}

std::vector<Object*>& Image::GetObjects(){
  return m_objects;
}

void Image::RemoveObject( Object* object ){
  std::vector<Object*>::iterator iter = std::find( m_objects.begin(), m_objects.end(), object );
  assert( iter != m_objects.end() );
  m_objects.erase(iter);
}

bool Image::HasObject( Object* obj ) const {
  return std::find(m_objects.begin(), m_objects.end(), obj) != m_objects.end();
}

void Image::Revert(){
  m_bitmap = m_original;
}

Bitmap* Image::GetBitmap(){
  return &m_bitmap;
}

const Bitmap& Image::GetBitmapRef() const{
  return m_bitmap;
}

Bitmap& Image::GetBitmapRef(){
  return m_bitmap;
}

}
