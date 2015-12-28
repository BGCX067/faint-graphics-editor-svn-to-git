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

#ifndef FAINT_IMAGE_LIST_HH
#define FAINT_IMAGE_LIST_HH
#include "util/index.hh"

namespace faint{
class Image;
class ImageProps;

class ImageList{
public:
  ImageList(ImageList&&);
  ImageList(ImageProps&&);
  ImageList(std::vector<ImageProps>&&);
  ~ImageList();
  Image& Active();
  const Image& Active() const;
  // Appends the image. The image will not be released when the
  // ImageList is destroyed.
  void AppendShared(Image*);
  index_t GetActiveIndex() const;
  Image& GetImage(const index_t&);
  Image& GetImage(const FrameId&);
  const Image& GetImage(const index_t&) const;
  const Image& GetImage(const FrameId&) const;
  index_t GetIndex(const Image&) const;
  index_t GetNumImages() const;
  bool Has(const Image*) const;
  bool Has(const FrameId& id) const;

  // Inserts the image at the specified index. The image will not be
  // released when the ImageList is destroyed.
  void InsertShared(Image*, const index_t&);
  void Remove(const index_t&);
  void Remove(Image*);
  void Reorder(const new_index_t&, const old_index_t&);
  void SetActiveIndex(const index_t&);

  ImageList(const ImageList&) = delete;
  ImageList& operator=(const ImageList&) = delete;
private:
  void InitAdd(ImageProps&&);
  index_t m_active = index_t(0);
  std::vector<Image*> m_images;
  std::vector<Image*> m_owned;
};

} // namespace faint
#endif
