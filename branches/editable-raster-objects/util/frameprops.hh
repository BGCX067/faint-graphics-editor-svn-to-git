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

#ifndef FAINT_FRAMEPROPS_HH
#define FAINT_FRAMEPROPS_HH
#include "bitmap/bitmap.hh"
#include "geo/geotypes.hh"
#include "util/color.hh"
#include "util/unique.hh"

class ImageInfo {
public:
  explicit ImageInfo( const IntSize& );
  ImageInfo( const IntSize&, const faint::Color& bgCol );
  IntSize size;
  faint::Color backgroundColor;
};

class category_frameinfo;
typedef Unique<IntPoint, category_frameinfo, 0> hot_spot_t;

class FrameInfo {
  // Properties for a single frame
public:
  explicit FrameInfo();
  explicit FrameInfo( const delay_t& );
  explicit FrameInfo( const hot_spot_t& );
  FrameInfo( const delay_t&, const hot_spot_t& );
  hot_spot_t::value_type hotSpot;
  delay_t::value_type delay;
};

class FrameProps{
public:
  typedef int ObjId;
  FrameProps();
  FrameProps(const faint::Bitmap&);
  FrameProps(const faint::Bitmap&, const FrameInfo&);
  FrameProps(const faint::Bitmap&, const objects_t&);
  FrameProps(const IntSize&, const objects_t&);
  FrameProps(const ImageInfo&);
  ~FrameProps();
  ObjId AddObject( Object* );
  // Stops the FrameProps from deleting the contents at destruction,
  // e.g. after creating an Image from the props.
  void Release();
  void RemoveObject( Object* );  
  const faint::Bitmap& GetBitmap() const;
  delay_t GetDelay() const;
  IntPoint GetHotSpot() const;
  const objects_t& GetObjects() const;
  Object* GetObject(ObjId);
  void SetBackgroundColor(const faint::Color&);
  faint::Color GetBackgroundColor() const;
  bool HasObject(ObjId) const;
  IntSize GetSize() const;
  bool HasBitmap() const;
  bool IsTopLevel( ObjId ) const;
  void SetBitmap( const faint::Bitmap& );
  void SetSize( const IntSize& );

private:
  objects_t m_allObjects;
  faint::Color m_bgColor;
  Optional<faint::Bitmap> m_bitmap;
  delay_t::value_type m_delay;
  hot_spot_t::value_type m_hotSpot;
  objects_t m_objects;
  bool m_owner;
  IntSize m_size;
};

#endif
