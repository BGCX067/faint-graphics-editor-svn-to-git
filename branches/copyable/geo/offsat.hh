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

#ifndef FAINT_OFFSAT_HH
#define FAINT_OFFSAT_HH
#include "geo/intpoint.hh"

namespace faint{

template<typename T>
class Offsat{
  // Wrapper for items that are offset relative to something else.
public:
  Offsat(const T& obj, const IntPoint& offset)
    : m_obj(obj),
      m_offset(offset)
  {}

  const T& Get() const{
    return m_obj;
  }

  T& Get(){
    return m_obj;
  }

  IntPoint Offset() const{
    return m_offset;
  }

  T& operator*(){
    return m_obj;
  }

  const T& operator*() const{
    return m_obj;
  }

  T* operator->(){
    return &m_obj;
  }
  const T* operator->() const{
    return &m_obj;
  }

private:
  T m_obj;
  IntPoint m_offset;
};

// Initialize an offsat
template<typename T>
Offsat<T> offsat(const T& obj, const IntPoint& offset){
  return Offsat<T>(obj, offset);
}

// Alternative (mostly because macros can't handle initializer list
// syntax)
template<typename T>
Offsat<T> offsat(const T& obj, int x, int y){
  return Offsat<T>(obj, {x,y});
}

} // namespace

#endif
