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

#ifndef FAINT_IMAGELIST_HH
#define FAINT_IMAGELIST_HH
#include "util/image.hh"

class category_imagelist;
typedef Unique<size_t, category_imagelist, 0> index_t;
bool operator<(const index_t&, const index_t&);
bool operator<=(const index_t&, const index_t&);
bool operator!=(const index_t&, const index_t&);
index_t operator-(const index_t&, const index_t&);
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
  Image& GetImageById(const FrameId&);
  const Image& GetImage(const index_t&) const;
  index_t GetIndex( const faint::Image& ) const;
  index_t GetNumImages() const;
  index_t GetSelectedIndex() const;
  bool Has(const faint::Image*) const;
  bool Has( const FrameId& id ) const;
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

} // namespace faint
#endif
