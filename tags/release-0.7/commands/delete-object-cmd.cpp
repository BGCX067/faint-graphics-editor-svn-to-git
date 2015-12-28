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

#include "commands/delete-object-cmd.hh"
#include "util/canvasinterface.hh"
#include "util/formatting.hh"

DelObjectCommand::DelObjectCommand( Object* object, size_t objectZ, const std::string& name )
  : Command( CMD_TYPE_OBJECT ),
    m_name(name),
    m_object( object ),
    m_objectZ( objectZ )
{}

void DelObjectCommand::Do( CommandContext& context ){
  context.Remove( m_object );
}

std::string DelObjectCommand::Name() const{
  return space_sep(m_name, m_object->GetType());
}

void DelObjectCommand::Undo( CommandContext& context ){
  context.Add( m_object, m_objectZ, select_added(false), deselect_old(false));
}
