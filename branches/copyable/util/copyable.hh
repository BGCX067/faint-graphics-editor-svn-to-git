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

#ifndef FAINT_COPYABLE_HH
#define FAINT_COPYABLE_HH
#include <type_traits>
#include <utility>

namespace faint{

template<typename T>
class CopySrc{
  // Wraps an object to allow another T
  // to be copy-constructed from it more explicitly.
  //
  // The CopySrc should be initialized with the copy-function.
  //
  // Example:
  //
  // SomeType t0("hello");
  // SomeType t1(copy(t0));

public:
  explicit CopySrc(const T& obj)
    : obj(obj)
  {
    static_assert(!(std::is_copy_constructible<T>::value),
    "Explicit copy on implicitly copyable object.");

  }

  CopySrc& operator=(const CopySrc&) = delete;
  const T& obj;
};

template<typename T>
CopySrc<T> copy(const T& obj){
  return CopySrc<T>(obj);
}

template<typename T>
class Copyable{
  // Makes
public:
  template<typename... Args>
  Copyable(Args... args)
    : obj(args...)
  {}

  Copyable(const T& other)
    : obj(copy(other))
  {}

  Copyable(T&& other)
    : obj(std::move(other))
  {}

  Copyable(const Copyable& other)
    : obj(copy(other.obj))
  {}

  Copyable(Copyable&& other)
    : obj(std::move(other.obj))
  {}

  Copyable& operator=(T&& other){
    obj = std::move(other);
    return *this;
  }

  operator T&(){
    return obj;
  }

  operator const T&() const{
    return obj;
  }

  operator CopySrc<T>() const{
    return copy(obj);
  }
  T obj;
};

template<typename T>
Copyable<T> make_copyable(T&& obj){
  return Copyable<T>(std::move(obj));
}

template<typename T>
CopySrc<T> copy(const Copyable<T>& copyable){
  return CopySrc<T>(copyable.obj);
}

} // namespace

#endif
