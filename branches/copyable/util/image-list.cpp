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

#include <algorithm>
#include "util/image.hh"
#include "util/image-list.hh"
#include "util/image-props.hh"

namespace faint{

ImageList::ImageList(ImageProps&& props){
  InitAdd(std::move(props));
}

ImageList::ImageList(ImageList&& source)
  : m_active(source.m_active),
    m_images(source.m_images),
    m_owned(source.m_owned)
{
  source.m_owned.clear();
  source.m_images.clear();
}

ImageList::ImageList(std::vector<ImageProps>&& props){
  for (ImageProps& p : props){
    InitAdd(std::move(p));
  }
}

ImageList::~ImageList(){
  for (Image* ownedImage : m_owned){
    delete ownedImage;
  }
  m_owned.clear();
  m_images.clear();
}

Image& ImageList::Active(){
  return *m_images[to_size_t(m_active.Get())];
}

const Image& ImageList::Active() const{
  return *m_images[to_size_t(m_active.Get())];
}

void ImageList::AppendShared(Image* image){
  m_images.push_back(image);
}

index_t ImageList::GetActiveIndex() const{
  return m_active;
}

index_t ImageList::GetIndex(const Image& img) const{
  for (size_t i = 0; i != m_images.size(); i++){
    if (m_images[i] == &img){
      return index_t(resigned(i));
    }
  }
  assert(false);
  return index_t(0);
}

Image& ImageList::GetImage(const index_t& index){
  assert(index < m_images.size());
  return *m_images[to_size_t(index.Get())];
}

const Image& ImageList::GetImage(const index_t& index) const{
  assert(valid_index(index, m_images));
  return *m_images[to_size_t(index)];
}

Image& ImageList::GetImageById(const FrameId& frameId){
  for (Image* image : m_images){
    if (image->GetId() == frameId){
      return *image;
    }
  }
  assert(false);
  return *m_images.front();
}

const Image& ImageList::GetImageById(const FrameId& frameId) const{
  return (const_cast<ImageList*>(this)->GetImageById(frameId));
}

index_t ImageList::GetNumImages() const{
  return index_t(resigned(m_images.size()));
}

bool ImageList::Has(const Image* image) const{
  return std::find(begin(m_images), end(m_images), image) != end(m_images);
}

bool ImageList::Has(const FrameId& frameId) const{
  for (Image* image : m_images){
    if (image->GetId() == frameId){
      return true;
    }
  }
  return false;
}

void ImageList::InitAdd(ImageProps&& props){
  for (int i = 0; i != props.GetNumFrames(); i++){
    Image* img = new Image(std::move(props.GetFrame(i)));
    m_owned.push_back(img);
    m_images.push_back(img);
  }
}

void ImageList::InsertShared(Image* image, const index_t& index){
  assert(std::find(begin(m_images), end(m_images), image) == end(m_images));
  m_images.insert(begin(m_images) + index.Get(), image);
}

void ImageList::Remove(Image* image){
  std::vector<Image*>::iterator it = std::find(begin(m_images), end(m_images), image);
  assert(it != m_images.end());
  m_images.erase(it);
  if (to_size_t(m_active) >= m_images.size()){
    m_active = index_t(resigned(m_images.size() - 1));
  }
}

void ImageList::Remove(const index_t& index){
  assert(valid_index(index, m_images));
  m_images.erase(begin(m_images) + index.Get());
  if (to_size_t(m_active) >= m_images.size()){
    m_active = index_t(resigned(m_images.size() - 1));
  }
}

void ImageList::Reorder(const new_index_t& newIndex, const old_index_t& oldIndex){
  assert(newIndex.Get() <= m_images.size());
  assert(oldIndex.Get() < m_images.size());
  if (newIndex.Get() == oldIndex.Get()){
    return;
  }
  int newRaw(newIndex.Get().Get());
  int oldRaw(oldIndex.Get().Get());
  Image* image = m_images[to_size_t(oldRaw)];
  m_images.erase(begin(m_images) + oldRaw);
  m_images.insert(begin(m_images) + newRaw, image);
}

void ImageList::SetActiveIndex(const index_t& index){
  m_active = index;
}

} // namespace faint
