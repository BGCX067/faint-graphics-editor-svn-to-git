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

#ifndef FAINT_PENDING_HH
#define FAINT_PENDING_HH

template<typename T>
T* transfer(T*& item ){
  T* newPtr = item;
  item = nullptr;
  return newPtr;
}

template <typename T>
class Pending {
  // Holds an item until it is retrieved or replaced. On destruction
  // (or replacement with Set) an un-retrieved item will be deleted.
public:
  Pending()
    : m_item(nullptr)
  {}
  ~Pending(){
    delete m_item;
  }
  void Set(T* item){
    delete m_item;
    m_item = item;
  }

  T* Retrieve(){
    return transfer(m_item);
  }

  bool Valid() const{
    return m_item != nullptr;
  }
private:
  T* m_item;
};

#endif
