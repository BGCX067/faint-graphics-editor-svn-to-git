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

#include "commands/order-object-cmd.hh"
#include "objects/object.hh"
#include "util/formatting.hh"

bool forward( const NewZ& newZ, const OldZ& oldZ ){
  return newZ.Get() > oldZ.Get();
}

std::string forward_or_back_str( const NewZ& newZ, const OldZ& oldZ ){
  return forward(newZ, oldZ) ? "Forward" : "Back";
}

OrderObjectCommand::OrderObjectCommand( Object* object, const NewZ& newZ, const OldZ& oldZ)
  : Command( CMD_TYPE_OBJECT ),
    m_object(object),
    m_newZ(newZ),
    m_oldZ(oldZ)
{}

void OrderObjectCommand::Do( CommandContext& context ){
  context.SetObjectZ( m_object, m_newZ.Get() );
}

std::string OrderObjectCommand::Name() const{
  return space_sep(m_object->GetType(), forward_or_back_str(m_newZ, m_oldZ));
}

void OrderObjectCommand::Undo( CommandContext& context ){
  context.SetObjectZ( m_object, m_oldZ.Get() );
}
