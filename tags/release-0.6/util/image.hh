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

#ifndef FAINT_IMAGE_HH
#define FAINT_IMAGE_HH
#include <cstddef>
#include <string>
#include <vector>
#include "bitmap/bitmap.hh"
#include "geo/geotypes.hh"
#include "util/rasterselection.hh"

class Object;

class ImageInfo {
public:
  explicit ImageInfo( const IntSize& );
  ImageInfo( const IntSize&, const faint::Color& bgCol );
  IntSize size;
  faint::Color backgroundColor;
};

class ImageProps;
typedef Unique<int, ImageProps, 0> delay_t;

class ImageProps{
  // For creating an image, e.g. during loading.
public:
  typedef int ObjId;
  ImageProps();
  ~ImageProps();
  ImageProps(const faint::Bitmap&);
  ImageProps(const faint::Bitmap&, const objects_t&);
  ImageProps(const IntSize&, const objects_t&);
  ImageProps(const ImageInfo&);
  ObjId AddObject( Object* );
  void AppendBitmap( const faint::Bitmap&, const delay_t& );
  size_t GetNumBitmaps() const;
  // Stops the ImageProps from deleting the contents at destruction,
  // e.g. after creating an Image from the props.
  void Release();
  void RemoveObject( Object* );
  faint::Bitmap GetBitmap() const;
  const faint::Bitmap& GetBitmap(size_t) const;
  int GetDelay(size_t num) const;
  std::string GetError() const;
  objects_t GetObjects() const;
  Object* GetObject(ObjId);
  void SetBackgroundColor(const faint::Color&);
  faint::Color GetBackgroundColor() const;
  bool HasObject(ObjId) const;
  IntSize GetSize() const;
  bool HasBitmap() const;
  bool IsBad() const;
  bool IsOk() const;
  bool IsTopLevel( ObjId ) const;
  void SetBitmap( const faint::Bitmap& );
  void SetSize( const IntSize& );
  void SetError( const std::string& );
private:
  bool m_ok;
  std::vector<faint::Bitmap> m_bitmaps;
  std::vector<int> m_delays;
  objects_t m_objects;
  objects_t m_allObjects;
  IntSize m_size;
  std::string m_error;
  bool m_owner;
  faint::Color m_bgColor;
};

namespace faint {
class Image {
public:
  explicit Image( ImageProps& );
  explicit Image( const faint::Bitmap& );
  Image( const Image& other );
  ~Image();
  void Add( Object* );
  void Add( Object*, size_t z );
  void ClearObjectSelection();
  RasterSelection& FloatRasterSelection( const copy_selected& );
  faint::Bitmap& GetBitmap();
  const faint::Bitmap& GetBitmap() const;
  size_t GetNumObjects() const;
  objects_t& GetObjects();
  objects_t GetObjects() const;
  size_t GetObjectZ( Object* ) const;
  const objects_t& GetObjectSelection() const;
  RasterSelection& GetRasterSelection();
  const RasterSelection& GetRasterSelection() const;
  IntSize GetSize() const;
  bool HasObject( Object* ) const;
  bool HasStoredOriginal();
  void Remove( Object* );
  void Revert();
  void SetBitmap( const faint::Bitmap& );
  void SetObjectZ( Object*, size_t z );
  void SetObjectSelection( const objects_t& );

  // Stores the current state as the original image
  // for Revert(). This is used when undoing, the image is
  // reverted and all raster commands reapplied.
  // StoreAsOriginal() must be called first.
  // This is not done on construction because it
  // would use a lot of memory when opening many images,
  // and should instead be called before applying the
  // first modification.
  void StoreAsOriginal();
private:
  objects_t m_objects;
  objects_t m_objectSelection;
  objects_t m_originalObjects;
  RasterSelection m_rasterSelection;
  faint::Bitmap m_bitmap;
  faint::Bitmap m_original;
  bool m_hasOriginal;
};

class ImageList{
public:
  ImageList( ImageProps& );
  ImageList( std::vector<ImageProps>& );
  ~ImageList();
  Image& Active();
  const Image& Active() const;
  void Add( faint::Image* );
  void Add( faint::Image*, const delay_t& );
  Image& GetImage(size_t);
  const Image& GetImage(size_t) const;
  int GetDelay(size_t) const;
  size_t GetNumImages() const;
  size_t GetSelectedIndex() const;
  void Insert( faint::Image*, size_t index ); // Fixme: Setting delay?
  void Remove(size_t index);
  void Remove(faint::Image*);
  void Reorder(size_t newIndex, size_t oldIndex);
  void SetDelay(size_t, const delay_t&);
  void SetSelectedIndex(size_t);
private:
  void InitAdd(ImageProps&);
  std::vector<Image*> m_images;
  std::vector<Image*> m_owned;
  std::vector<int> m_delays; // Fixme: Group with the images as pairs
  size_t m_selected;
};

IntRect image_rect( const Image& );

}
#endif
