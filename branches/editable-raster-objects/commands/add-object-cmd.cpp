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

#include "commands/add-object-cmd.hh"
#include "objects/object.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"

AddObjectCommand::AddObjectCommand( Object* object, const select_added& select, const std::string& name )
  : Command( CommandType::OBJECT ),
    m_object(object),
    m_select(select.Get()),
    m_name(name)
{}

AddObjectCommand::~AddObjectCommand(){
  delete m_object;
}

void AddObjectCommand::Do( CommandContext& context ){
  context.Add( m_object, select_added(then_false(m_select)), deselect_old(false));
}

void AddObjectCommand::Undo( CommandContext& context ){
  context.Remove(m_object);
}

std::string AddObjectCommand::Name() const{
  return space_sep(m_name, m_object->GetType());
}
