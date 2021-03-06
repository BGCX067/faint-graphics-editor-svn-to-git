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

#ifndef FAINT_CMDCHANGESETTING_HH
#define FAINT_CMDCHANGESETTING_HH
#include "command.hh"
#include "object.hh"

template<typename T>
class CmdChangeSetting : public Command {
public:
  CmdChangeSetting( Object* object, const T& setting, const typename T::ValueType value )
    : Command( CMD_TYPE_OBJECT ),
      m_object( object ),
      m_setting( setting ),
      m_newValue( value )

  {
    m_oldValue = object->GetSettings().Get( setting );
  }

  virtual void Do( faint::Image& ){
    m_object->Set( m_setting, m_newValue );
  }

  virtual void Undo( faint::Image& ){
    m_object->Set( m_setting, m_oldValue );
  }

private:
  Object* m_object;
  T m_setting;
  typename T::ValueType m_newValue;
  typename T::ValueType m_oldValue;
};

#endif
