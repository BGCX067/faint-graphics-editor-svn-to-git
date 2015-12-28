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

#include "tricommand.hh"
#include "objects/object.hh"

TriCommand::TriCommand( Object* object, const Tri& newTri, const Tri& oldTri )
  : Command( CMD_TYPE_OBJECT ),
    m_object( object ),
    m_new( newTri ),
    m_old( oldTri )
{}

void TriCommand::Do( faint::Image& ){
  m_object->SetTri( m_new );
}

void TriCommand::Undo( faint::Image& ){
  m_object->SetTri( m_old );
}
