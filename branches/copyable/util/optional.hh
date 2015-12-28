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

#ifndef FAINT_OPTIONAL_HH
#define FAINT_OPTIONAL_HH
#include <cassert>
#include <utility>

namespace faint{

template<typename T>
class Optional{
  // Wrapper for optionally specified class members, return types and
  // parameters.
  //
  // When instantiated with the one-parameter constructor, IsSet()
  // will return true, and the Get-functions will return the
  // member. When default-constructed, the getters will instead
  // assert. This is less error prone than using pointers and not as
  // bulky as a boolean and a member.
public:
  Optional()
    : m_obj(nullptr)
  {}


  template<typename=typename std::enable_if<std::is_copy_constructible<T>::value>::type>
  explicit Optional(const T& obj) :
    m_obj(new T(obj))
  {}

  Optional(T&& obj) :
    m_obj(new T(std::move(obj)))
  {}

  Optional(Optional&& other)
  {
    m_obj = other.m_obj;
    other.m_obj = nullptr;
  }

  template<typename= typename std::enable_if<std::is_copy_constructible<T>::value>::type>
  Optional(const Optional<T>& other){
    if (other.m_obj == nullptr){
      m_obj = nullptr;
    }
    else {
      m_obj = new T(*other.m_obj);
    }
  }

  ~Optional(){
    delete m_obj;
    m_obj = nullptr;
  }

  template<typename= typename std::enable_if<std::is_copy_constructible<T>::value>::type>
  Optional& operator=(const Optional& other){
    if ( this == &other ){
      return *this;
    }

    delete m_obj;
    if ( other.m_obj == nullptr ){
      m_obj = nullptr;
    }
    else{
      m_obj = new T(*other.m_obj);
    }
    return *this;
  }

  Optional& operator=(Optional&& other){
    if (this == &other){
      return *this;
    }

    delete m_obj;
    if (other.m_obj == nullptr){
      m_obj = nullptr;
    }
    else{
      m_obj = other.m_obj;
      other.m_obj = nullptr;
    }
    return *this;
  }

  void Clear(){
    delete m_obj;
    m_obj = nullptr;
  }

  T& Get(){
    assert(m_obj != nullptr);
    return *m_obj;
  }

  const T& Get() const{
    assert(m_obj != nullptr);
    return *m_obj;
  }

  T Take(){
    assert(m_obj != nullptr);
    T value(std::move(*m_obj));
    Clear();
    return value;
  }

  bool IsSet() const{
    return m_obj != nullptr;
  }

  bool NotSet() const{
    return m_obj == nullptr;
  }

  const T& Or(const T& alternative) const{
    return m_obj == nullptr ?
      alternative : *m_obj;
  }

  void Set(const T& obj){
    delete m_obj;
    m_obj = new T(obj);
  }

  void Set(T&& obj){
    delete m_obj;
    m_obj = new T(std::move(obj));
  }

  template<typename FUNC>
  void Visit(const FUNC& whenSet) const{
    if (m_obj != nullptr){
      whenSet(*m_obj);
    }
  }

  template<typename FUNC1, typename FUNC2>
  auto Visit(const FUNC1& whenSet, const FUNC2& otherwise) const
    -> decltype(otherwise())
  {
    return m_obj != nullptr ?
      whenSet(*m_obj) : otherwise();
  }

  // Fixme: See if this can be resolved on lack of callability,
  // rather than suffixing Simple
  template<typename FUNC, typename RET>
  RET VisitSimple(const FUNC& whenSet, const RET& otherwise) const{
    return m_obj != nullptr ?
      whenSet(*m_obj) : otherwise;
  }

  explicit operator bool() const{
    return m_obj != nullptr;
  }
private:
  T* m_obj;
};

template<typename T>
bool operator==( const Optional<T>& obj, const T& other ){
  return obj.IsSet() && obj.Get() == other;
}

template<typename T>
bool operator!=( const Optional<T>& obj, const T& other ){
  return obj.NotSet() || obj.Get() != other;
}

template<typename T>
Optional<T> option( const T& obj ){
  return Optional<T>(obj);
}

// This class allows returning an uninitialized Optional from a
// function without having to repeat the type.
class no_option{
public:
  template<typename T>
  operator Optional<T>(){
    return Optional<T>();
  }
};

template<typename T>
bool optional_eq( const Optional<T>& o, const T& value ){
  return o.IsSet() && o.Get() == value;
}

template<typename T>
T& operator|=(T& lhs, const Optional<T>& rhs){
  if (rhs.IsSet()){
    lhs = rhs.Get();
  }
  return lhs;
}

template<typename T>
Optional<T>& operator|=(Optional<T>& lhs, const Optional<T>& rhs){
  if (rhs.IsSet()){
    lhs.Set(rhs.Get());
  }
  return lhs;
}

} // namespace

#endif
