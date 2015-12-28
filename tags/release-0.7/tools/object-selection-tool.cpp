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

#include "app/getappcontext.hh"
#include "commands/change-setting-cmd.hh" // Fixme: Here or task?
#include "commands/command-bunch.hh"
#include "commands/update-object-settings-cmd.hh" // Fixme: Here or task?
#include "tasks/select-object-idle.hh"
#include "tools/object-selection-tool.hh"
#include "util/commandutil.hh"
#include "util/objutil.hh" // Fixme: move to task

// Creates a single object update setting or a CommandBunch if multiple objects are affected
// Fixme: Move to commands/?
template<typename T>
Command* UpdateObjectSettings( const objects_t& objects, const T& s, typename T::ValueType value ){
  if ( objects.empty() ){
    return 0;
  }

  std::vector<Command*> commands;
  for ( objects_t::const_iterator it = objects.begin(); it != objects.end(); ++it ){
    if ( (*it)->GetSettings().Has(s) ){
      commands.push_back( new CmdChangeSetting<T>( *it, s, value ) );
    }
  }
  if ( !commands.empty() ){
    return perhaps_bunch(CMD_TYPE_OBJECT, bunch_name(commands.back()->Name()), commands);
  }
  return 0;
}

void ObjSettingNotifier::Notify( const BoolSetting& s, bool value ){
  Command* cmd = UpdateObjectSettings( GetAppContext().GetActiveCanvas().GetObjectSelection(), s, value );
  RunCommand( cmd );
}
void ObjSettingNotifier::Notify( const IntSetting& s, int value ){
  Command* cmd = UpdateObjectSettings( GetAppContext().GetActiveCanvas().GetObjectSelection(), s, value );
  RunCommand( cmd );

}
void ObjSettingNotifier::Notify( const StrSetting& s, const std::string& value ){
  Command* cmd = UpdateObjectSettings( GetAppContext().GetActiveCanvas().GetObjectSelection(), s, value );
  RunCommand( cmd );
}
void ObjSettingNotifier::Notify( const ColorSetting& s, const faint::Color& value ){
  Command* cmd = UpdateObjectSettings( GetAppContext().GetActiveCanvas().GetObjectSelection(), s, value );
  RunCommand( cmd );
}
void ObjSettingNotifier::Notify( const FloatSetting& s, FloatSetting::ValueType value ){
  Command* cmd = UpdateObjectSettings( GetAppContext().GetActiveCanvas().GetObjectSelection(), s, value );
  RunCommand( cmd );
}

void ObjSettingNotifier::RunCommand( Command* cmd ){
  if ( cmd ){
    GetAppContext().GetActiveCanvas().RunCommand( cmd );
  }
}

ObjSelectTool::ObjSelectTool() :
  MultiTool( T_OBJ_SEL,
    m_notifier,
    EAT_SETTINGS,
    default_task(new SelectObjectIdle(m_settings)))
{
  m_settings = get_object_settings(GetAppContext().GetActiveCanvas().GetObjectSelection());
}
