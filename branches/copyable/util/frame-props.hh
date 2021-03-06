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

#ifndef FAINT_FRAME_PROPS_HH
#define FAINT_FRAME_PROPS_HH
#include "bitmap/bitmap-fwd.hh"
#include "geo/geo-fwd.hh"
#include "geo/intpoint.hh"
#include "geo/intsize.hh"
#include "util/color.hh"
#include "util/distinct.hh"
#include "util/optional.hh"

namespace faint{

class category_imageinfo;
typedef Distinct<bool, category_imageinfo, 0> create_bitmap;
class ImageInfo {
public:
  explicit ImageInfo(const IntSize&, const create_bitmap&);
  ImageInfo(const IntSize&, const Color& bg, const create_bitmap&);
  IntSize size;
  Color backgroundColor;
  bool createBitmap;
};

class category_frameinfo;
typedef Distinct<IntPoint, category_frameinfo, 0> hot_spot_t;

class FrameInfo {
  // Properties for a single frame
public:
  explicit FrameInfo();
  explicit FrameInfo(const delay_t&);
  explicit FrameInfo(const hot_spot_t&);
  FrameInfo(const delay_t&, const hot_spot_t&);
  delay_t::value_type delay;
  hot_spot_t::value_type hotSpot;
};

class FrameProps{
public:
  typedef int ObjId;
  FrameProps();
  FrameProps(FrameProps&&);
  FrameProps(const FrameProps&) = delete;
  explicit FrameProps(const Bitmap&);
  explicit FrameProps(const ImageInfo&);
  FrameProps(const Bitmap&, const FrameInfo&);
  FrameProps(const Bitmap&, const objects_t&);
  FrameProps(const IntSize&, const objects_t&);
  ~FrameProps();
  ObjId AddObject(Object*);
  // Stops the FrameProps from deleting the contents at destruction,
  // e.g. after creating an Image from the props.
  void Release();
  void RemoveObject(Object*);
  Bitmap& GetBitmap();
  delay_t GetDelay() const;
  IntPoint GetHotSpot() const;
  const objects_t& GetObjects() const;
  Object* GetObject(ObjId);
  void SetBackgroundColor(const Color&);
  Color GetBackgroundColor() const;
  bool HasObject(ObjId) const;
  IntSize GetSize() const;
  bool HasBitmap() const;
  bool IsTopLevel(ObjId) const;
  void SetBitmap(const Bitmap&);
  void SetSize(const IntSize&);
private:
  objects_t m_allObjects;
  Color m_bgColor;
  Optional<Bitmap> m_bitmap;
  delay_t::value_type m_delay;
  hot_spot_t::value_type m_hotSpot;
  objects_t m_objects;
  bool m_owner;
  IntSize m_size;
};

} // namespace

#endif
