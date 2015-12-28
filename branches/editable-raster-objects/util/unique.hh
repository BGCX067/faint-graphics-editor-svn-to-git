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

#ifndef FAINT_UNIQUE_HH
#define FAINT_UNIQUE_HH
#include <cassert>

template<typename T, typename CATEGORY, int UNIQUE_ID>
class Unique{
  // Promoter of parameter incompatibility.
  // T is the wrapped type, CATEGORY provides further separation
  // than the UNIQUE_ID.

  // Example,
  // Avoid confusing the message and the title in message boxes:
  // class message_part{}; // category
  // > typedef Unique<std::string, message_part, 0> Title;
  // > typedef Unique<std::string, message_part, 1> Message;
  // > void Message(const Title&, const Message&);
public:
  explicit Unique(const T& obj) :
    m_obj(obj)
  {}
  T& Get(){
    return m_obj;
  }
  const T& Get() const{
    return m_obj;
  }
  bool operator==(const Unique<T, CATEGORY, UNIQUE_ID>& other ) const{
    return m_obj == other.m_obj;
  }
  typedef T value_type;
private:
  T m_obj;
};

template<typename T, int UNIQUE_ID>
class LessUnique{
  // Promoter of parameter incompatibility.
  // T is the wrapped type, the UNIQUE_ID ensures uniqueness
  // (without the categorizing of the Unique-template).
public:
  explicit LessUnique(const T& obj) :
    m_obj(obj)
  {}
  T& Get(){
    return m_obj;
  }
  const T& Get() const{
    return m_obj;
  }
  bool operator==(const LessUnique<T, UNIQUE_ID>& other ) const{
    return m_obj == other.m_obj;
  }
private:
  T m_obj;
};

template<typename T>
class Target{
public:
  explicit Target(T& obj)
    : m_obj(obj)
  {}
  T& Get() const{
    return m_obj;
  }
private:
  Target& operator=(const Target&);
  T& m_obj;
};

template<typename T>
class from_t{
public:
  explicit from_t(T& container) :
    m_container(container)
  {}

  T& Get() const{
    return m_container;
  }
private:
  from_t<T>& operator=(const from_t<T>&);
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
private:
  these_t<T>& operator=(const these_t<T>&);
  T& m_container;
};

template<typename T>
these_t<T> these(T& container){
  return these_t<T>(container);
}

template <typename T>
struct Order{
  // Parameter ordering, when the parameters represent something new
  // and something old, to avoid mixing them up.

  // Implementation note: Template-typedef idiom, C++11 supports alias
  // instead, but it's not available yet in VC.
  // For instantiating, just use the New and Old-template functions
  typedef Unique<T, Order, 0> New;
  typedef Unique<T, Order, 1> Old;
};

template<typename T>
typename Order<T>::Old Old(const T& t){
  return typename Order<T>::Old(t);
}

template<typename T>
typename Order<T>::New New(const T& t){
  return typename Order<T>::New(t);
}

template<typename T>
struct Alternative{
  typedef Unique<T, Alternative, 0> Type;
};

template<typename T>
typename Alternative<T>::Type Alternate(const T& t){
  return typename Alternative<T>::Type(t);
}

template<typename T>
class Optional{
  // For optional class members. When instantiated with the
  // one-parameter constructor, IsSet() will return true, and the
  // Get-functions will return the member. When default-constructed,
  // the getters will instead assert. This is less error prone
  // than using pointers and not as bulky as a boolean and a member.
public:
  explicit Optional(const T& obj) :
    m_obj(new T(obj))
  {}

  Optional()
    : m_obj(nullptr)
  {}

  Optional( const Optional& other ){
    if ( other.m_obj == nullptr ){
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

  Optional& operator=( const Optional& other ){
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

  bool IsSet() const{
    return m_obj != nullptr;
  }

  bool NotSet() const{
    return m_obj == nullptr;
  }

  T& Get(){
    assert(m_obj != nullptr);
    return *m_obj;
  }

  const T& Get() const{
    assert(m_obj != nullptr);
    return *m_obj;
  }

  void Set(const T& obj){
    delete m_obj;
    m_obj = new T(obj);
  }

  void Clear(){
    delete m_obj;
    m_obj = nullptr;
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

template <typename T>
struct Boundary{
  typedef Unique<T, Boundary, 0> Min;
  typedef Unique<T, Boundary, 1> Max;
};

template<typename T>
typename Boundary<T>::Min Min(const T& t){
  return typename Boundary<T>::Min(t);
}

template<typename T>
typename Boundary<T>::Max Max(const T& t){
  return typename Boundary<T>::Max(t);
}

#endif
