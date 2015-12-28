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

#ifndef FAINT_DISTINCT_HH
#define FAINT_DISTINCT_HH
#include <utility> // for std::move

template<typename T, typename CATEGORY, int ID>
class Distinct{
  // Promoter of parameter incompatibility by deriving distinct types.
  //
  // T is the wrapped type, CATEGORY provides further separation
  // than the ID.
  //
  // Example,
  // Avoid confusing the message and the title in message boxes
  // class category_message;
  // > typedef Distinct<std::string, category_message, 0> Title;
  // > typedef Distinct<std::string, category_message, 1> Message;
  // > void Message(const Title&, const Message&);
public:
  explicit Distinct(const T& obj) :
    m_obj(obj)
  {}

  explicit Distinct(T&& obj) :
    m_obj(std::move(obj))
  {}

  T& Get(){
    return m_obj;
  }

  const T& Get() const{
    return m_obj;
  }

  bool operator==(const Distinct<T, CATEGORY, ID>& other ) const{
    return m_obj == other.m_obj;
  }

  typedef T value_type;
private:
  T m_obj;
};

// Lesser-promoter of parameter incompatibility.  T is the wrapped
// type, the ID provides distinction (without the categorizing of the
// Distinct-template).
class category_common;
template<typename T, int ID>
using LessDistinct = Distinct<T, category_common, ID>;

template <typename T>
struct Order{
  // Types for parameter ordering, when two same-type parameters
  // represent something new and something old, to avoid mixing them
  // up.
  //
  // For instantiating, use the New and Old function templates outside
  // this struct
  typedef Distinct<T, Order, 0> New;
  typedef Distinct<T, Order, 1> Old;
};

template<typename T>
typename Order<T>::Old Old(const T& t){
  return typename Order<T>::Old(t);
}

template<typename T>
typename Order<T>::New New(const T& t){
  return typename Order<T>::New(t);
}

class category_alternative;
template<typename T>
using Alternative = Distinct<T, category_alternative, 0>;

template<typename T>
Alternative<T> Alternate(const T& t){
  return Alternative<T>(t);
}

template <typename T>
struct Boundary{
  typedef Distinct<T, Boundary, 0> Min;
  typedef Distinct<T, Boundary, 1> Max;
};

template<typename T>
typename Boundary<T>::Min Min(const T& t){
  return typename Boundary<T>::Min(t);
}

template<typename T>
typename Boundary<T>::Max Max(const T& t){
  return typename Boundary<T>::Max(t);
}

template<typename T>
class from_t{
  // Class for wrapping a container function parameter with a
  // from-preposition. Used with the from function template.
  //
  // Example:
  // > remove(someThings, from(myContainer);
public:
  explicit from_t(T& container) :
    m_container(container)
  {}

  T& Get() const{
    return m_container;
  }

  from_t<T>& operator=(const from_t<T>&) = delete;

private:
  T& m_container;
};

template<typename T>
from_t<T> from(T& container){
  return from_t<T>(container);
}

template<typename T>
class these_t{
public:
  explicit these_t(T& container) :
    m_container(container)
  {}

  T& Get() const{
    return m_container;
  }

  these_t<T>& operator=(const these_t<T>&) = delete;
private:
  T& m_container;
};

template<typename T>
these_t<T> these(T& container){
  return these_t<T>(container);
}
template<typename T>
class its_yours_t{
public:
  explicit its_yours_t(T* obj) : m_obj(obj)
  {}
  T* Get() const{
    return m_obj;
  }
private:
  T* m_obj;
};

template<typename T>
class just_a_loan_t{
public:
  explicit just_a_loan_t(T* obj) : m_obj(obj)
  {}
  T* Get() const{
    return m_obj;
  }
private:
  T* m_obj;
};

template<typename T>
its_yours_t<T> its_yours( T* ptr ){
  return its_yours_t<T>(ptr);
}

template<typename T>
just_a_loan_t<T> just_a_loan( T* ptr ){
  return just_a_loan_t<T>(ptr);
}

#endif
