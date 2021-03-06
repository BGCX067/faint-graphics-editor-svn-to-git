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

#ifndef FAINT_EITHER_HH
#define FAINT_EITHER_HH
#include "util/optional.hh"

namespace faint{
template<typename T1, typename T2>
class Either{
  // Return type for one or the other.
public:
  Either(const Either& other)
    : m_v1(other.m_v1),
      m_v2(other.m_v2)
  {}

  template<typename= typename std::enable_if<std::is_copy_constructible<T1>::value>::type>
  Either(const T1& v1)
    : m_v1(v1)
  {}

  Either(T1&& v1)
    : m_v1(std::move(v1))
  {}

  Either(T2&& v2)
    : m_v2(std::move(v2))
  {}

  template<typename=typename std::enable_if<std::is_copy_constructible<T2>::value>::type>
  Either(const T2& v2)
    : m_v2(v2)
  {}

  template<typename F1, typename F2>
  auto Visit(const F1& f1, const F2& f2) const -> decltype(f1(*((T1*)nullptr))){
    if (m_v1.IsSet()){
      return f1(m_v1.Get());
    }
    else{
      return f2(m_v2.Get());
    }
  }

  template<typename F1, typename F2>
  auto Visit(const F1& f1, const F2& f2) -> decltype(f1(*((T1*)nullptr))){
    if (m_v1.IsSet()){
      return f1(m_v1.Get());
    }
    else{
      return f2(m_v2.Get());
    }
  }

  template<typename T>
  T& Expect(){
    return Get<T>().Get();
  }

  template<typename T>
  const T& Expect() const{
    return Get<T>().Get();
  }

  template<typename=typename std::enable_if<std::is_copy_constructible<T1>::value>::type>
  void Set(const T1& v1){
    m_v1.Set(v1);
    m_v2.Clear();
  }
  template<typename=typename std::enable_if<std::is_copy_constructible<T2>::value>::type>
  void Set(const T2& v2){
    m_v2.Set(v2);
    m_v1.Clear();
  }

  void Set(T1&& v1){
    m_v1.Set(std::move(v1));
    m_v2.Clear();
  }

  void Set(T2&& v2){
    m_v2.Set(std::move(v2));
    m_v1.Clear();
  }

  template<typename T>
  Optional<T>& Get(){
    Optional<T>* opt;
    DoGet(opt);
    return *opt;
  }

  template<typename T>
  const Optional<T>& Get() const{
    const Optional<T>* opt;
    DoGet(opt);
    return *opt;
  }

private:
  void DoGet(Optional<T1>*& opt){
    opt = &m_v1;
  }
  void DoGet(Optional<T2>*& opt){
    opt = &m_v2;
  }

  void DoGet(const Optional<T1>*& opt) const{
    opt = &m_v1;
  }
  void DoGet(const Optional<T2>*& opt) const{
    opt = &m_v2;
  }

  Optional<T1> m_v1;
  Optional<T2> m_v2;
};

} // namespace

#endif
