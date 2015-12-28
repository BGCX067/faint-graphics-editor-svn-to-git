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

#ifndef FAINT_ITER_HH
#define FAINT_ITER_HH
#include "util/common-fwd.hh"

namespace faint{

template<typename T>
class ContainerType{
  // The non-specialized ContainerType stores a container by
  // reference, as the container is assumed to live past the iteration
  // scope.
  // The specializations for wrappers (e.g. but_first_t) instead store
  // wrappers by value, as they would expire immediately otherwise.
public:
  ContainerType(const T& container)
    : c(container)
  {}
  const T& c;
  ContainerType& operator=(const ContainerType&) = delete;
};

template<typename Container>
class reversed_t {
private:
  ContainerType<Container> m_container;
public:
  reversed_t(const Container& c)
    : m_container(c)
  {}

  auto begin() const -> decltype(m_container.c.rbegin()) {
    return m_container.c.rbegin();
  }

  auto end() const -> decltype(m_container.c.rend()) {
    return m_container.c.rend();
  }
  typedef typename Container::value_type value_type;
  reversed_t& operator=(const reversed_t&) = delete;
};


template<typename T>
class ContainerType<reversed_t<T> >{
public:
  ContainerType(const reversed_t<T>& container)
    : c(container)
  {}
  const reversed_t<T> c;
  ContainerType& operator=(const ContainerType&) = delete;
};


template<typename Container>
reversed_t<Container> reversed(const Container& container) {
  return reversed_t<Container>(container);
}

inline reversed_t<objects_t> top_to_bottom(const objects_t& objects){
  return reversed_t<objects_t>(objects);
}

template<typename Container>
class but_first_t {
private:
  const ContainerType<Container> m_container;
public:
  but_first_t(const Container& c)
    : m_container(c)
  {}
  auto begin() const -> decltype(m_container.c.begin()) {
    return m_container.c.empty() ? m_container.c.end() : m_container.c.begin() + 1;
  }
  auto end() const -> decltype(m_container.c.end()) {
    return m_container.c.end();
  }

  typedef typename Container::value_type value_type;
  but_first_t& operator=(const but_first_t&) = delete;
};

template<typename T>
class ContainerType<but_first_t<T> >{
public:
  ContainerType(const but_first_t<T>& container)
    : c(container)
  {}
  const but_first_t<T> c;
  ContainerType& operator=(const ContainerType&) = delete;
};

template<typename Container>
but_first_t<Container> but_first(const Container& container) {
  return but_first_t<Container>(container);
}

template<typename Container>
class but_last_t {
private:
  const ContainerType<Container> m_container;
public:
  but_last_t(const Container& c)
    : m_container(c)
  {}
  auto begin() -> decltype(m_container.c.begin()) {
    return m_container.c.begin();
  }
  auto end() -> decltype(m_container.c.end()) {
    return m_container.c.empty() ?
      m_container.c.end() :
      m_container.c.begin() + resigned(m_container.c.size()) - 1;
  }

  auto rbegin() const -> decltype(m_container.c.rbegin()) {
    return m_container.c.empty() ?
      m_container.c.rend() :
      m_container.c.rbegin() + 1;
  }

  auto rend() const -> decltype(m_container.c.rend()) {
    return m_container.c.crend();
  }

  typedef typename Container::value_type value_type;
private:
  but_last_t& operator=(const but_last_t&) = delete;
};

template<typename T>
class ContainerType<but_last_t<T> >{
public:
  ContainerType(const but_last_t<T>& container)
    : c(container)
  {}
  const but_last_t<T> c;
private:
  ContainerType& operator=(const ContainerType&) = delete;
};

template<typename Container>
but_last_t<Container> but_last(const Container& container) {
  return but_last_t<Container>(container);
}

template<typename T>
class enum_iter_t{
  // Iterator for enums values.
public:
  enum_iter_t(T value)
    : m_value(static_cast<T2>(value))
  {}

  T operator*() const{
    return static_cast<T>(m_value);
  }

  void operator++(){
    m_value++;
  }

  bool operator!=( const enum_iter_t& rhs ) const{
    return m_value != rhs.m_value;
  }
private:
  typedef typename std::underlying_type<T>::type T2;
  T2 m_value;
};

template<typename T>
class iterable{
  // Wrapper which makes enums with BEGIN and END members iterable.
public:
  iterable(){}

  enum_iter_t<T> begin(){
    return enum_iter_t<T>(T::BEGIN);
  }
  enum_iter_t<T> end(){
    return enum_iter_t<T>(T::END);
  }
private:
};

template<typename T>
class CountedItem{
public:
  CountedItem(int in_num, const T& in_item)
    : num(in_num),
      item(in_item)
  {}

  const T& operator*() const{
    return item;
  }

  const int num;
  const T& item;
private:
  CountedItem& operator=(const CountedItem&) = delete;
};

template<typename ITER_T, typename VALUE_T>
class enumerate_iter_t{
private:
  ITER_T m_iter;
  int m_num;
public:
  enumerate_iter_t(ITER_T iter):
    m_iter(iter),
    m_num(0)
  {}

  auto operator*() -> CountedItem<VALUE_T>{
    return CountedItem<VALUE_T>(m_num, *m_iter);
  }

  enumerate_iter_t operator++(){
    m_iter++;
    m_num++;
    return *this;
  }

  bool operator!=(const enumerate_iter_t& other){
    return m_iter != other.m_iter;
  }
};

template<typename Container>
class enumerate_t{
private:
  const ContainerType<Container> m_container;
public:
  enumerate_t(const Container& c)
    : m_container(c)
  {}

  auto begin() const -> enumerate_iter_t<decltype(m_container.c.begin()), typename Container::value_type> {
    return enumerate_iter_t<decltype(m_container.c.begin()), typename Container::value_type>(m_container.c.begin());
  }

  auto end() const -> enumerate_iter_t<decltype(m_container.c.end()), typename Container::value_type> {
    return enumerate_iter_t<decltype(m_container.c.end()), typename Container::value_type>(m_container.c.end());
  }
private:
  enumerate_t& operator=(const enumerate_t&) = delete;
};


template<typename Container>
enumerate_t<Container> enumerate(const Container& container) {
  return enumerate_t<Container>(container);
}

} // namespace

#endif
