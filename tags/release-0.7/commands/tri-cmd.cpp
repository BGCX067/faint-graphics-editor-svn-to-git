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

#include "commands/tri-cmd.hh"
#include "objects/object.hh"

TriCommand::TriCommand( Object* object, const NewTri& newTri, const OldTri& oldTri, const std::string& name )
  : Command( CMD_TYPE_OBJECT ),
    m_object( object ),
    m_new( newTri.Get() ),
    m_old( oldTri.Get() ),
    m_name(name)
{}

void TriCommand::Do( CommandContext& ){
  m_object->SetTri( m_new );
}

std::string TriCommand::Name() const{
  return m_name + " " + m_object->GetType();
}

void TriCommand::Undo( CommandContext& ){
  m_object->SetTri( m_old );
}
