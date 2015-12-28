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

#ifndef FAINT_UNDO_HH
#define FAINT_UNDO_HH
template <typename T>
class UndoRedo{
public:
  UndoRedo(){
  }

  UndoRedo( const T& initial ){
    m_undoList.push_back(initial);
  }

  void Clear(){
    m_undoList.clear();
    m_redoList.clear();
  }

  void Did( const T& t ){
    m_undoList.push_back(t);
    m_redoList.clear();
  }

  bool CanUndo() const{
    return !m_undoList.empty();
  }

  bool CanRedo() const{
    return !m_redoList.empty();
  }

  const T& Undo(){
    assert( CanUndo() );
    m_redoList.push_back(m_undoList.back());
    m_undoList.pop_back();
    return m_redoList.back();
  }

  const T& Redo(){
    assert( CanRedo() );
    m_undoList.push_back(m_redoList.back());
    m_redoList.pop_back();
    return m_undoList.back();
  }

  const T& PeekRedo() const{
    assert( CanRedo());
    return m_redoList.back();
  }

  const T& PeekUndo() const{
    assert( CanUndo() );
    return m_undoList.back();
  }
private:
  std::vector<T> m_undoList;
  std::vector<T> m_redoList;
};

#endif
