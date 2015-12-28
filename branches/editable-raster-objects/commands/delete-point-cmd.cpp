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

#include "commands/delete-point-cmd.hh"
#include "objects/object.hh"
#include "util/formatting.hh"

DeletePointCommand::DeletePointCommand( Object* object, size_t pointIndex )
  : Command( CommandType::OBJECT ),
    m_object(object),
    m_pointIndex(pointIndex),
    m_point(object->GetPoint(pointIndex))
{}

void DeletePointCommand::Do( CommandContext& ){
  m_object->RemovePoint(m_pointIndex);
}

std::string DeletePointCommand::Name() const{
  return space_sep("Delete",  m_object->GetType(), "Point");
}

void DeletePointCommand::Undo( CommandContext& ){
  m_object->InsertPoint(m_point, m_pointIndex);
}
