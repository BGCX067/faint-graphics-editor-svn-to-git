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

#include "commands/drawobjectcommand.hh"
#include "objects/object.hh"
#include "util/formatting.hh"

DrawObjectCommand::DrawObjectCommand( const its_yours_t<Object>& obj )
  : Command( CMD_TYPE_RASTER ),
    m_delete(true),
    m_object(obj.Get())
{}

DrawObjectCommand::DrawObjectCommand( const just_a_loan_t<Object>& obj )
  : Command( CMD_TYPE_RASTER ),
    m_delete(false),
    m_object(obj.Get())
{}

DrawObjectCommand::~DrawObjectCommand(){
  if ( m_delete ){
    delete m_object;
  }
}

void DrawObjectCommand::Do( CommandContext& context ){
  m_object->Draw( context.GetDC() );
}

std::string DrawObjectCommand::Name() const{
  return space_sep("Draw ", m_object->GetType());
}
