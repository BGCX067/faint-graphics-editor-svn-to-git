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

#include <cassert>
#include "util/index.hh"

namespace faint{

index_t::index_t() :
  m_value(0)
{}

index_t::index_t(int value)
  : m_value(value)
{
  assert(m_value >= 0);
}

int index_t::Get() const{
  return m_value;
}

bool operator==(const index_t& lhs, const index_t& rhs){
  return lhs.Get() == rhs.Get();
}

index_t& index_t::operator=(const index_t& rhs){
  m_value = rhs.Get();
  return *this;
}

bool operator<(const index_t& lhs, const index_t& rhs){
  return lhs.Get() < rhs.Get();
}

bool operator>(const index_t& lhs, size_t rhs){
  return to_size_t(lhs.Get()) > rhs;
}

bool operator<=(const index_t& lhs, const index_t& rhs ){
  return lhs.Get() <= rhs.Get();
}

bool operator!=(const index_t& lhs, const index_t& rhs){
  return lhs.Get() != rhs.Get();
}

bool operator<(const index_t& lhs, size_t rhs){
  return to_size_t(lhs.Get()) < rhs;
}

bool operator<=(const index_t& index, size_t rhs){
  size_t lhs = to_size_t(index);
  return lhs <= rhs;
}

index_t operator-(const index_t& index, size_t rhs){
  size_t lhs = to_size_t(index.Get());
  assert(lhs >= rhs);
  return index_t(resigned(lhs - rhs));
}

size_t to_size_t(const index_t& index){
  return to_size_t(index.Get());
}

} // namespace
