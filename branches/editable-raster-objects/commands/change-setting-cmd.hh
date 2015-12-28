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
#include "commands/command.hh"
#include "objects/object.hh"
#include "util/formatting.hh"

template<typename T>
class CmdChangeSetting : public Command {
public:
  CmdChangeSetting( Object* object, const T& setting, const typename T::ValueType& value )
    : Command( CommandType::OBJECT ),
      m_object( object ),
      m_setting( setting ),
      m_newValue( value )
  {
    m_oldValue = object->GetSettings().Get( setting );
  }
  std::string Name() const override{
    return space_sep("Change", m_object->GetType(), setting_name_pretty(untyped(m_setting)));
  }

  void Do( CommandContext& ) override{
    m_object->Set( m_setting, m_newValue );
  }

  void Undo( CommandContext& ) override{
    m_object->Set( m_setting, m_oldValue );
  }

private:
  CmdChangeSetting& operator=(const CmdChangeSetting&);
  Object* m_object;
  T m_setting;
  typename T::ValueType m_newValue;
  typename T::ValueType m_oldValue;
};

template<typename T>
CmdChangeSetting<T>* change_setting_command( Object* obj, const T& setting, const typename T::ValueType& value ){
  return new CmdChangeSetting<T>(obj, setting, value);
}

#endif
