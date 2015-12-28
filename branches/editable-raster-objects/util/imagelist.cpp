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
#include "util/imagelist.hh"
#include "util/imageprops.hh"

bool operator<(const index_t& lhs, const index_t& rhs){
  return lhs.Get() < rhs.Get();
}

bool operator<=(const index_t& lhs, const index_t& rhs ){
  return lhs.Get() <= rhs.Get();
}

bool operator!=(const index_t& lhs, const index_t& rhs){
  return lhs.Get() != rhs.Get();
}

index_t operator-(const index_t& index, size_t rhs){
  size_t lhs = index.Get();
  assert(lhs >= rhs);
  return index_t(lhs - rhs);
}

namespace faint{

ImageList::ImageList( ImageProps& props )
  : m_selected(0)
{
  InitAdd(props);
}

ImageList::ImageList( std::vector<ImageProps>& props )
  : m_selected(0)
{
  for ( ImageProps& p : props ){
    InitAdd(p);
  }
}

ImageList::~ImageList(){
  for ( Image* ownedImage : m_owned ){
    delete ownedImage;
  }
  m_owned.clear();
  m_images.clear();
}

Image& ImageList::Active(){
  return *m_images[m_selected.Get()];
}

const Image& ImageList::Active() const{
  return *m_images[m_selected.Get()];
}

void ImageList::Add( faint::Image* image ){
  m_images.push_back(image);
}

index_t ImageList::GetIndex( const faint::Image& img ) const{
  for ( size_t i = 0; i != m_images.size(); i++ ){
    if ( m_images[i] == &img ){
      return index_t(i);
    }
  }
  assert(false);
  return index_t(0);
}

Image& ImageList::GetImage(const index_t& index){
  assert(index.Get() < m_images.size());
  return *m_images[index.Get()];
}

const Image& ImageList::GetImage(const index_t& index) const{
  assert(index.Get() < m_images.size());
  return *m_images[index.Get()];
}

Image& ImageList::GetImageById( const FrameId& frameId ){
  for ( Image* image : m_images ){
    if ( image->GetId() == frameId ){
      return *image;
    }
  }
  assert(false);
  return *m_images.front();
}

index_t ImageList::GetNumImages() const{
  return index_t(m_images.size());
}

index_t ImageList::GetSelectedIndex() const{
  return m_selected;
}

bool ImageList::Has(const faint::Image* image ) const{
  return std::find(m_images.begin(), m_images.end(), image ) != m_images.end();
}

bool ImageList::Has(const FrameId& frameId ) const{
  for ( faint::Image* image : m_images ){
    if ( image->GetId() == frameId ){
      return true;
    }
  }
  return false;
}

void ImageList::InitAdd(ImageProps& props){
  for ( size_t i = 0; i != props.GetNumFrames(); i++ ){
    faint::Image* img = new Image(props.GetFrame(i));
    m_owned.push_back(img);
    m_images.push_back(img);
  }
}

void ImageList::Insert(faint::Image* image, const index_t& index){
  assert(std::find(m_images.begin(), m_images.end(), image) == m_images.end());
  m_images.insert(m_images.begin() + index.Get(), image);
}

void ImageList::Remove(faint::Image* image){
  std::vector<faint::Image*>::iterator it = std::find(m_images.begin(), m_images.end(), image);
  assert( it != m_images.end() );
  m_images.erase(it);
  if ( m_selected.Get() >= m_images.size() ){
    m_selected = index_t(m_images.size() - 1);
  }
}

void ImageList::Remove(const index_t& index){
  assert(index.Get() < m_images.size());
  m_images.erase(m_images.begin() + index.Get());
  if ( m_selected.Get() >= m_images.size() ){
    m_selected = index_t(m_images.size() - 1);
  }
}

void ImageList::Reorder(const new_index_t& newIndex, const old_index_t& oldIndex){
  assert(newIndex.Get() <= index_t(m_images.size()));
  assert(oldIndex.Get() < index_t(m_images.size()));
  if ( newIndex.Get() == oldIndex.Get() ){
    return;
  }
  size_t newRaw(newIndex.Get().Get());
  size_t oldRaw(oldIndex.Get().Get());
  faint::Image* image = m_images[oldRaw];
  m_images.erase(m_images.begin() + oldRaw);
  m_images.insert(m_images.begin() + newRaw, image);
}

void ImageList::SetSelected( const index_t& index ){
  m_selected = index;
}

} // namespace faint
