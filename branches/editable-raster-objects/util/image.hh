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
#include <vector>
#include "bitmap/bitmap.hh"
#include "geo/geotypes.hh"
#include "util/color.hh"
#include "util/idtypes.hh"
#include "util/frameprops.hh"
#include "util/rasterselection.hh"
#include "util/unique.hh"
class Object;

namespace faint {
class Image {
public:
  explicit Image( FrameProps& );
  explicit Image( const faint::Bitmap& );
  Image( const Image& other );
  ~Image();
  void Add( Object* );
  void Add( Object*, size_t z );
  bool Deselect(const Object*);
  bool Deselect(const objects_t& );
  void DeselectObjects();
  RasterSelection& FloatRasterSelection( const copy_selected& );
  faint::Bitmap& GetBitmap();
  const faint::Bitmap& GetBitmap() const;

  // The time to remain on this image if part of an animation
  delay_t GetDelay() const;
  IntPoint GetHotSpot() const;
  FrameId GetId() const;
  size_t GetNumObjects() const;
  const objects_t& GetObjects() const;
  size_t GetObjectZ( const Object* ) const;
  const objects_t& GetObjectSelection() const;
  RasterSelection& GetRasterSelection();
  const RasterSelection& GetRasterSelection() const;
  IntSize GetSize() const;
  bool Has( const ObjectId& ) const;
  bool HasObject( const Object* ) const; // Fixme: Rename (without Object)
  bool HasStoredOriginal();
  void Remove( Object* );
  void Revert();

  // Select the specified objects, optionally deselecting
  // the current selection.
  // The objects must exist in this image.
  void SelectObjects( const objects_t& );
  void SetBitmap( const faint::Bitmap& );
  void SetDelay(const delay_t&);
  void SetHotSpot(const IntPoint&);
  void SetObjectZ( Object*, size_t z );

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
  IntPoint m_hotSpot;
  FrameId m_id;
  objects_t m_objects;
  objects_t m_objectSelection;
  faint::Bitmap m_original;
  objects_t m_originalObjects;
  RasterSelection m_rasterSelection;
};

// Rectangle with the same size as the image, anchored at 0,0
IntRect image_rect( const Image& );

} // namespace faint

#endif
