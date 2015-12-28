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

#include "pointcommand.hh"
#include "objects/object.hh"

PointCommand::PointCommand( Object* object, size_t pointIndex, const Point& newPoint, const Point& oldPoint )
  : Command( CMD_TYPE_OBJECT ),
    m_object( object ),
    m_pointIndex( pointIndex ),
    m_new( newPoint ),
    m_old( oldPoint )
{}

void PointCommand::Do( faint::Image& ){
  m_object->SetPoint( m_new, m_pointIndex );
}

void PointCommand::Undo( faint::Image& ){
  m_object->SetPoint( m_old, m_pointIndex );
}
