// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_INDEX_HH
#define FAINT_INDEX_HH
#include "geo/primitive.hh"
#include "util/distinct.hh"

namespace faint{

class index_t{
public:
  index_t();
  explicit index_t(int);
  explicit index_t(size_t);
  int Get() const;
  index_t& operator=(const index_t& rhs);
  index_t& operator++();
private:
  int m_value;
};

bool operator==(const index_t&, const index_t&);
bool operator==(const index_t&, const size_t&);
bool operator<(const index_t&, const index_t&);
bool operator>(const index_t&, size_t);
bool operator<=(const index_t&, const index_t&);
bool operator!=(const index_t&, const index_t&);
bool operator!=(const index_t&, size_t);
index_t operator-(const index_t&, const index_t&);
index_t operator-(const index_t&, size_t);
bool operator<=(const index_t&, size_t);
bool operator<(const index_t&, size_t);
using new_index_t = Order<index_t>::New;
using old_index_t = Order<index_t>::Old;
size_t to_size_t(const index_t&);
index_t to_index(size_t);

template<typename T>
bool valid_index(const index_t& index, const T& container){
  return to_size_t(index.Get()) <= container.size();
}

} // namespace

#endif
