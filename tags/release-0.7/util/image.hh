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
#include "util/unique.hh"

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
  delay_t GetDelay(size_t num) const;
  std::string GetError() const;
  const objects_t& GetObjects() const;
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
  // The time to remain on this image if part of an animation
  delay_t GetDelay() const;
  size_t GetNumObjects() const;
  const objects_t& GetObjects() const;
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
  void SetDelay(const delay_t&);
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
  faint::Bitmap m_bitmap;
  delay_t m_delay;
  bool m_hasOriginal;
  objects_t m_objects;
  objects_t m_objectSelection;
  faint::Bitmap m_original;
  objects_t m_originalObjects;
  RasterSelection m_rasterSelection;
};
} // namespace faint

typedef Unique<size_t, faint::Image, 0> index_t;
bool operator<(const index_t&, const index_t&);
bool operator!=(const index_t&, const index_t&);
index_t operator-(const index_t&, size_t);
typedef Order<index_t>::New new_index_t;
typedef Order<index_t>::Old old_index_t;

namespace faint{

class ImageList{
public:
  ImageList( ImageProps& );
  ImageList( std::vector<ImageProps>& );
  ~ImageList();
  Image& Active();
  const Image& Active() const;
  void Add( faint::Image* );
  Image& GetImage(const index_t&);
  const Image& GetImage(const index_t&) const;
  index_t GetNumImages() const;
  index_t GetSelectedIndex() const;
  bool Has(const faint::Image*) const;
  void Insert( faint::Image*, const index_t& );
  void Remove(const index_t&);
  void Remove(faint::Image*);
  void Reorder(const new_index_t&, const old_index_t&);
  void SetSelected(const index_t&);
private:
  void InitAdd(ImageProps&);
  std::vector<Image*> m_images;
  std::vector<Image*> m_owned;
  index_t m_selected;
};

IntRect image_rect( const Image& );

}
#endif
