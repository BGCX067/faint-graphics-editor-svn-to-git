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

#include "addpointcommand.hh"
#include "objects/object.hh"

AddPointCommand::AddPointCommand( Object* object, size_t pointIndex, const Point& point )
  : Command( CMD_TYPE_OBJECT ),
    m_object(object),
    m_pointIndex(pointIndex),
    m_point(point)
{}

void AddPointCommand::Do(CommandContext&){
  m_object->InsertPoint(m_point, m_pointIndex);
}

std::string AddPointCommand::Name() const{
  return std::string("Add Point to ") + m_object->GetType();
}

void AddPointCommand::Undo(CommandContext&){
  m_object->RemovePoint(m_pointIndex);
}
