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

#include "bitmap/bitmap.hh"
#include "objects/object.hh"
#include "util/frame-props.hh"
#include "util/object-util.hh"

namespace faint{

ImageInfo::ImageInfo(const IntSize& in_size, const create_bitmap& in_createBitmap)
  : size(in_size),
    backgroundColor(color_white()),
    createBitmap(in_createBitmap.Get())
{}

ImageInfo::ImageInfo(const IntSize& in_size, const Color& bgCol, const create_bitmap& in_createBitmap)
  : size(in_size),
    backgroundColor(bgCol),
    createBitmap(in_createBitmap.Get())
{}

FrameInfo::FrameInfo()
  : delay(0),
    hotSpot(0,0)
{}

FrameInfo::FrameInfo(const delay_t& in_delay)
  : delay(in_delay.Get()),
    hotSpot(0,0)
{}

FrameInfo::FrameInfo(const hot_spot_t& in_hotSpot)
  : delay(0),
    hotSpot(in_hotSpot.Get())
{}

FrameInfo::FrameInfo(const delay_t& in_delay, const hot_spot_t& in_hotSpot)
  : delay(in_delay.Get()),
    hotSpot(in_hotSpot.Get())
{}

FrameProps::FrameProps()
  : m_bgColor(255,255,255),
    m_owner(true),
    m_size(640,480)
{}

FrameProps::FrameProps(const Bitmap& bmp)
  : m_bgColor(255,255,255),
    m_owner(true),
    m_size(640,480)
{
  m_bitmap.Set(copy(bmp));
  m_delay = 0;
  m_hotSpot = IntPoint(0,0);
}

FrameProps::FrameProps(const Bitmap& bmp, const objects_t& objects)
  : m_bgColor(255,255,255),
    m_objects(objects),
    m_owner(true),
    m_size(640,480)
{
  m_bitmap.Set(copy(bmp));
  m_delay = 0;
  m_hotSpot = IntPoint(0,0);
}

FrameProps::FrameProps(const Bitmap& bmp, const FrameInfo& info)
  : m_bgColor(255,255,255),
    m_delay(info.delay),
    m_hotSpot(info.hotSpot),
    m_owner(true),
    m_size(640,480)
{
  m_bitmap.Set(copy(bmp));
}

FrameProps::FrameProps(const IntSize& size, const objects_t& objects)
  : m_bgColor(255,255,255),
    m_objects(objects),
    m_owner(true),
    m_size(size)
{}

FrameProps::FrameProps(const ImageInfo& info)
  : m_bgColor(info.backgroundColor),
    m_owner(true),
    m_size(info.size)
{
  if (info.createBitmap){
    m_bitmap.Set(Bitmap(m_size, m_bgColor));
  }
}

FrameProps::FrameProps(FrameProps&& other)
  : m_allObjects(std::move(other.m_allObjects)),
    m_bgColor(other.m_bgColor),
    m_bitmap(std::move(other.m_bitmap)),
    m_delay(other.m_delay),
    m_hotSpot(other.m_hotSpot),
    m_objects(std::move(other.m_objects)),
    m_owner(true),
    m_size(other.m_size)
{}

FrameProps::~FrameProps(){
  if (m_owner){
    for (Object* obj : m_objects){
      // Not deleting from m_allObjects, since the ObjComposites
      // are responsible for deleting sub-objects.
      delete obj;
    }
  }
  m_objects.clear();
  m_allObjects.clear();
}

FrameProps::ObjId FrameProps::AddObject(Object* obj){
  m_objects.push_back(obj);
  m_allObjects.push_back(obj);
  return static_cast<ObjId>(m_allObjects.size()) - 1;
}

Bitmap& FrameProps::GetBitmap(){
  return m_bitmap.Get();
}

delay_t FrameProps::GetDelay() const{
  return delay_t(m_delay);
}

IntPoint FrameProps::GetHotSpot() const{
  return m_hotSpot;
}

Object* FrameProps::GetObject(FrameProps::ObjId id){
  return m_allObjects.at(to_size_t(id));
}

const objects_t& FrameProps::GetObjects() const{
  return m_objects;
}

IntSize FrameProps::GetSize() const{
  return m_bitmap.NotSet() ? m_size : m_bitmap.Get().GetSize();
}

bool FrameProps::HasBitmap() const{
  return m_bitmap.IsSet();
}

bool FrameProps::HasObject(FrameProps::ObjId id) const{
  const ObjId maxId(static_cast<ObjId>(m_allObjects.size()));
  return 0 <= id && id < maxId;
}

bool FrameProps::IsTopLevel(ObjId id) const{
  assert(HasObject(id));
  Object* obj = m_allObjects.at(to_size_t(id));
  return contains(m_objects, obj);
}

void FrameProps::Release(){
  m_owner = false;
}

void FrameProps::SetBackgroundColor(const Color& color){
  m_bgColor = color;
}

Color FrameProps::GetBackgroundColor() const{
  return m_bgColor;
}

void FrameProps::RemoveObject(Object* obj){
  bool removed = remove(obj, from(m_objects));
  assert(removed);
}

void FrameProps::SetBitmap(const Bitmap& bmp){
  m_bitmap.Set(copy(bmp));
  m_delay = 0;
  m_hotSpot = IntPoint(0,0);
}

void FrameProps::SetSize(const IntSize& size){
  // Fixme: Do what if has bitmap?
  m_size = size;
}

} // namespace
