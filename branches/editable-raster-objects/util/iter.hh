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
#include "util/commonfwd.hh"

template<typename Container>
class reversed_t {
private:
  const Container& m_container;
public:
  reversed_t(const Container& c)
    : m_container(c)
  {}
  auto begin() -> decltype(m_container.rbegin()) {
    return m_container.rbegin();
  }
  auto end() -> decltype(m_container.rend()) {
    return m_container.rend();
  }
private:
  reversed_t& operator=(const reversed_t&);
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
  const Container& m_container;
public:
  but_first_t(const Container& c)
    : m_container(c)
  {}
  auto begin() -> decltype(m_container.begin()) {
    return m_container.empty() ? m_container.end() : m_container.begin() + 1;
  }
  auto end() -> decltype(m_container.end()) {
    return m_container.end();
  }
private:
  but_first_t& operator=(const but_first_t&);
};

template<typename Container>
but_first_t<Container> but_first(const Container& container) {
  return but_first_t<Container>(container);
}

template<typename Container>
class but_last_t {
private:
  const Container& m_container;
public:
  but_last_t(const Container& c)
    : m_container(c)
  {}
  auto begin() -> decltype(m_container.begin()) {
    return m_container.begin();
  }
  auto end() -> decltype(m_container.end()) {
    return m_container.empty() ?
      m_container.end() :
      m_container.begin() + m_container.size() - 1;
  }
private:
  but_last_t& operator=(const but_last_t&);
};

template<typename Container>
but_last_t<Container> but_last(const Container& container) {
  return but_last_t<Container>(container);
}

template<typename T>
class enum_iter_t{
public:
  enum_iter_t(T value)
    : m_value(static_cast<int>(value)) // Fixme: use underlying_type
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
  int m_value;
};

template<typename T>
class iterable{
public:
  iterable()
    : m_value(static_cast<int>(T::BEGIN))
  {}

  enum_iter_t<T> begin(){
    return enum_iter_t<T>(T::BEGIN);
  }
  enum_iter_t<T> end(){
    return enum_iter_t<T>(T::END);
  }
private:
  int m_value; // Fixme: Use underlying type
};

#endif
