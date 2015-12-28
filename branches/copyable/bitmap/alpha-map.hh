// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_ALPHA_MAP_HH
#define FAINT_ALPHA_MAP_HH
#include "geo/intsize.hh"
#include "util/common-fwd.hh"
#include "util/copyable.hh"

namespace faint{

class AlphaMapRef{
public:
  uchar Get(int x, int y) const;
  IntSize GetSize() const;
private:
  friend class AlphaMap;
  AlphaMapRef(const faint::uchar*, const IntSize&, int);
  const faint::uchar* m_data;
  IntSize m_size;
  int m_stride;
};

class AlphaMap{
public:
  explicit AlphaMap(const IntSize&);
  ~AlphaMap();
  AlphaMap(AlphaMap&&);
  AlphaMap(const CopySrc<AlphaMap>&);
  void Add(int x, int y, faint::uchar value);
  AlphaMapRef FullReference() const;
  uchar Get(int x, int y) const;
  faint::uchar* GetRaw();
  const faint::uchar* GetRaw() const;
  IntSize GetSize() const;
  void Reset(const IntSize&);
  void Set(int x, int y, faint::uchar value);
  AlphaMapRef SubReference(const IntRect&) const;
  AlphaMap SubCopy(const IntRect&) const;

  AlphaMap& operator=(const AlphaMap&) = delete;
private:
  // Fixme, private for now due to:
  // http://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
  AlphaMap(const AlphaMap&) = delete;
  void Initialize(const IntSize&);
  void Clear();
  faint::uchar* m_data;
  IntSize m_size;
  int m_stride;
};

void stroke(AlphaMap&, const IntPoint&, const IntPoint&, const Brush&);
} // namespace

#endif
