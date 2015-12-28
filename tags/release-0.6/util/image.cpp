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
#include "rendering/cairocontext.hh"
#include "util/image.hh"
#include "util/objutil.hh"

ImageInfo::ImageInfo( const IntSize& in_size )
  : size(in_size),
    backgroundColor(faint::white_color())
{}

ImageInfo::ImageInfo( const IntSize& in_size, const faint::Color& bgCol )
  : size(in_size),
    backgroundColor(bgCol)
{}

ImageProps::ImageProps()
  : m_ok(true),
    m_size(640,480),
    m_owner(true),
    m_bgColor(255,255,255)
{}

ImageProps::ImageProps(const faint::Bitmap& bmp )
  : m_ok(true),
    m_size(640,480),
    m_owner(true),
    m_bgColor(255,255,255)
{
  m_bitmaps.push_back(bmp);
  m_delays.push_back(0);
}

ImageProps::ImageProps( const faint::Bitmap& bmp, const objects_t& objects )
  : m_ok(true),
    m_objects(objects),
    m_size(640,480),
    m_owner(true),
    m_bgColor(255,255,255)
{
  m_bitmaps.push_back(bmp);
  m_delays.push_back(0);
}

ImageProps::ImageProps( const IntSize& size, const objects_t& objects )
  : m_ok(true),
    m_objects(objects),
    m_size(size),
    m_owner(true),
    m_bgColor(255,255,255)
{}

ImageProps::ImageProps( const ImageInfo& info )
  : m_ok(true),
    m_size(info.size),
    m_owner(true),
    m_bgColor(info.backgroundColor)
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

void ImageProps::AppendBitmap(const faint::Bitmap& bmp, const delay_t& delay){
  m_bitmaps.push_back(bmp);
  m_delays.push_back(delay.Get());
}

size_t ImageProps::GetNumBitmaps() const{
  return m_bitmaps.size();
}

faint::Bitmap ImageProps::GetBitmap() const{
  assert(m_ok);
  assert(!m_bitmaps.empty());
  return m_bitmaps.front();
}

const faint::Bitmap& ImageProps::GetBitmap(size_t num) const{
  assert(m_ok);
  assert(num < m_bitmaps.size());
  return m_bitmaps[num];
}

int ImageProps::GetDelay(size_t num) const{
  assert(m_ok);
  assert(num < m_delays.size());
  return m_delays[num];
}

std::string ImageProps::GetError() const{
  assert(!m_ok);
  return m_error;
}

objects_t ImageProps::GetObjects() const{
  return m_objects;
}

Object* ImageProps::GetObject(ImageProps::ObjId id){
  return m_allObjects[id];
}

IntSize ImageProps::GetSize() const{
  return m_bitmaps.empty() ? m_size : m_bitmaps.front().GetSize();
}

bool ImageProps::HasBitmap() const{
  return !m_bitmaps.empty();
}

bool ImageProps::HasObject( ImageProps::ObjId id ) const{
  const ObjId maxId(static_cast<ObjId>(m_allObjects.size()));
  return 0 <= id && id < maxId;
}

bool ImageProps::IsBad() const{
  return !m_ok;
}

bool ImageProps::IsOk() const{
  return m_ok;
}

bool ImageProps::IsTopLevel( ObjId id ) const{
  assert( HasObject(id) );
  Object* obj = m_allObjects.at(id);
  return contains(m_objects, obj);
}

void ImageProps::Release(){
  m_owner = false;
}

void ImageProps::SetBackgroundColor(const faint::Color& color){
  m_bgColor = color;
}

faint::Color ImageProps::GetBackgroundColor() const{
  return m_bgColor;
}

void ImageProps::RemoveObject( Object* obj ){
  bool removed = remove(obj, from(m_objects));
  assert(removed);
}

void ImageProps::SetBitmap( const faint::Bitmap& bmp ){
  m_bitmaps.clear();
  m_delays.clear();
  m_bitmaps.push_back(bmp);
  m_delays.push_back(0);
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
    m_originalObjects(m_objects),
    m_hasOriginal(false)
{
  if ( props.HasBitmap() ){
    m_bitmap = props.GetBitmap();
  }
  else {
    m_bitmap = cairo_compatible_bitmap( props.GetSize() );
    clear(m_bitmap, props.GetBackgroundColor());
  }
  // "The object memory is now mine" - Image.
  props.Release();
}

Image::Image( const faint::Bitmap& bmp )
  : m_bitmap(bmp),
    m_hasOriginal(false)
{}

Image::Image( const Image& other )
  : m_bitmap(other.m_bitmap),
    m_hasOriginal(false)
{
  // Fixme: Copy objects
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
  for ( size_t i = 0; i != m_originalObjects.size(); i++ ){
    delete m_originalObjects[i];
  }
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

RasterSelection& Image::FloatRasterSelection( const copy_selected& copy ){
  m_rasterSelection.BeginFloat(m_bitmap, copy);
  return m_rasterSelection;
}

objects_t Image::GetObjects() const{ // Fixme: const-reference instead?
  return m_objects;
}

const objects_t& Image::GetObjectSelection() const{
  return m_objectSelection;
}

size_t Image::GetObjectZ( Object* obj ) const {
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

void Image::SetObjectSelection( const objects_t& objects ){
  m_objectSelection = objects;
  assert(contains(m_objects, these(m_objectSelection)));
}

void Image::SetObjectZ( Object* obj, size_t z ){
  Remove( obj );
  z = std::min( z, m_objects.size() );
  m_objects.insert( m_objects.begin() + z, obj ); // Fixme: Check if correct
}

objects_t& Image::GetObjects(){
  return m_objects;
}

void Image::Remove( Object* object ){
  bool removed(remove(object, from(m_objects)));
  assert(removed);
}

size_t Image::GetNumObjects() const{
  return m_objects.size();
}

bool Image::HasObject( Object* obj ) const {
  return contains(m_objects, obj);
}

void Image::Revert(){
  assert(m_hasOriginal);
  m_bitmap = m_original;
}

const Bitmap& Image::GetBitmap() const{
  return m_bitmap;
}

Bitmap& Image::GetBitmap(){
  return m_bitmap;
}

IntRect image_rect( const Image& image ){
  return IntRect(IntPoint(0,0), image.GetSize());
}

ImageList::ImageList( ImageProps& props ){
  InitAdd(props);
  m_selected = 0;
}

ImageList::ImageList( std::vector<ImageProps>& props ){
  for ( size_t i = 0; i != props.size(); i++ ){
    InitAdd(props[i]);
  }
  m_selected = 0;
}

ImageList::~ImageList(){
  for ( size_t i = 0; i != m_owned.size(); i++ ){
    delete m_owned[i];
  }
  m_owned.clear();
  m_images.clear();
}

Image& ImageList::Active(){
  return *m_images[m_selected];
}

const Image& ImageList::Active() const{
  return *m_images[m_selected];
}

void ImageList::Add( faint::Image* image, const delay_t& delay ){
  m_images.push_back(image);
  m_delays.push_back(delay.Get());
}

void ImageList::Add( faint::Image* image ){
  Add(image, delay_t(0));
}

int ImageList::GetDelay(size_t index) const{
  return m_delays[index];
}

Image& ImageList::GetImage(size_t index){
  assert(index < m_images.size());
  return *m_images[index];
}

const Image& ImageList::GetImage(size_t index) const{
  assert(index < m_images.size());
  return *m_images[index];
}

size_t ImageList::GetNumImages() const{
  return m_images.size();
}

size_t ImageList::GetSelectedIndex() const{
  return m_selected;
}

void ImageList::InitAdd(ImageProps& props){
  if ( props.GetNumBitmaps() <= 1 ){
    faint::Image* img = new Image(props);
    m_owned.push_back(img);
    m_images.push_back(img);
    if ( props.GetNumBitmaps() != 0 ){
      m_delays.push_back(props.GetDelay(0));
    }
    else {
      m_delays.push_back(0);
    }
  }
  else {
    for ( size_t i = 0; i != props.GetNumBitmaps(); i++ ){
      faint::Image* img = new Image(props.GetBitmap(i));
      m_owned.push_back(img);
      m_images.push_back(img);
      m_delays.push_back(props.GetDelay(i));
    }
  }
}

void ImageList::Insert(faint::Image* image, size_t index){
  assert(std::find(m_images.begin(), m_images.end(), image) == m_images.end());
  m_images.insert(m_images.begin() + index, image);
  m_delays.insert(m_delays.begin() + index, 0);
}

void ImageList::Remove(faint::Image* image){
  std::vector<faint::Image*>::iterator it = std::find(m_images.begin(), m_images.end(), image);
  assert( it != m_images.end() );
  size_t delta = it - m_images.begin();
  m_images.erase(it);
  if ( m_selected >= m_images.size() ){
    m_selected = m_images.size() - 1;
  }
  m_delays.erase(m_delays.begin() + delta);
}

void ImageList::Remove(size_t index){
  assert(index < m_images.size());
  m_images.erase(m_images.begin() + index);
  if ( m_selected >= m_images.size() ){
    m_selected = m_images.size() - 1;
  }
  m_delays.erase(m_delays.begin() + index);
}

void ImageList::Reorder(size_t newIndex, size_t oldIndex){
  assert(newIndex <= m_images.size());
  assert(oldIndex < m_images.size());
  if ( newIndex == oldIndex ){
    return;
  }
  faint::Image* image = m_images[oldIndex];
  m_images.erase(m_images.begin() + oldIndex);
  int delay = m_delays[oldIndex];
  m_delays.erase(m_delays.begin() + oldIndex);
  if ( newIndex > oldIndex ){
    newIndex -= 1;
  }
  m_images.insert(m_images.begin() + newIndex, image);
  m_delays.insert(m_delays.begin() + newIndex, delay);
}

void ImageList::SetDelay( size_t num, const delay_t& delay ){
  assert(num < m_delays.size() );
  m_delays[num] = delay.Get();
}

void ImageList::SetSelectedIndex( size_t num ){
  m_selected = num;
}

}
