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
#ifndef FAINT_DEREFITER_HH
#define FAINT_DEREFITER_HH

namespace faint{

template<typename T>
class deref_iter{
  // Iterator template which dereferences one step on operator->, to
  // allow less annoying iteration of pointers in containers.
  //
  // Note: T is the container type, not the iterator type.
public:
  explicit deref_iter( const typename T::iterator& it ) : m_it(it)
  {}
  bool operator!=( const typename T::iterator& it){
    return m_it != it;
  }
  void operator++(){
    ++m_it;
  }
  typename T::value_type operator->(){
    return *m_it;
  }
  operator typename T::value_type(){
    return *m_it;
  }
private:
  typename T::iterator m_it;
};

template<typename T>
class const_deref_iter{
public:
  explicit const_deref_iter( const typename T::const_iterator& it ) : m_it(it)
  {}
  bool operator!=( const typename T::const_iterator& it){
    return m_it != it;
  }
  void operator++(){
    ++m_it;
  }
  typename T::value_type operator->(){
    return *m_it;
  }
  operator typename T::value_type(){
    return *m_it;
  }
private:
  typename T::const_iterator m_it;
};
}
#endif
