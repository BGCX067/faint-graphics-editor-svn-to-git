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

#ifndef FAINTIMAGE_HH
#define FAINTIMAGE_HH
#include <cstddef>
#include <string>
#include <vector>
#include "bitmap/bitmap.h"
#include "geo/geotypes.hh"
class Object;

// For creating an image, e.g. during loading.
class ImageProps{
public:
  typedef int ObjId;
  ImageProps();
  ~ImageProps();
  ImageProps(const faint::Bitmap&);
  ImageProps(const faint::Bitmap&, const std::vector<Object*>&);
  ObjId AddObject( Object* );
  // Stops the ImageProps from deleting the contents at destruction,
  // e.g. after creating an Image from the props.
  void Release();
  void RemoveObject( Object* );
  faint::Bitmap GetBitmap() const;
  std::string GetError() const;
  std::vector<Object*> GetObjects() const;
  Object* GetObject(ObjId);
  faint::Color GetBackgroundColor() const;
  bool HasObject(ObjId) const;
  IntSize GetSize() const;
  bool HasBitmap() const;
  bool IsOk() const;
  bool IsTopLevel( ObjId ) const;
  void SetBitmap( const faint::Bitmap& );
  void SetSize( const IntSize& );
  void SetError( const std::string& );
private:
  bool m_ok;
  bool m_hasBitmap;
  faint::Bitmap m_bitmap;
  std::vector<Object*> m_objects;
  std::vector<Object*> m_allObjects;
  IntSize m_size;
  std::string m_error;
  bool m_owner;
};

namespace faint {
class Image {
public:
  Image( ImageProps& );
  Image( const IntSize&, const std::vector<Object*>& );
  Image( const faint::Bitmap& );
  Image( const faint::Bitmap&, const std::vector<Object*>& );
  ~Image();
  void Revert();
  void AddObject( Object* );
  void AddObject( Object*, size_t z );
  void SetObjectZ( Object*, size_t z );
  int GetObjectZ( Object* ) const;
  void RemoveObject( Object* );
  bool HasObject( Object* ) const;
  IntSize GetSize() const;
  std::vector<Object*>& GetObjects();
  std::vector<Object*> GetObjects() const;
  faint::Bitmap* GetBitmap();
  faint::Bitmap& GetBitmapRef();
  const faint::Bitmap& GetBitmapRef() const;
  void SetBitmap( const faint::Bitmap& );
  void SetBaseBitmap( const faint::Bitmap& );
private:
  std::vector<Object*> m_objects;
  std::vector<Object*> m_originalObjects;
  faint::Bitmap m_bitmap;
  faint::Bitmap m_original;
};



}
#endif
