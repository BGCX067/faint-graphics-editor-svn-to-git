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

#include <cassert>
#include "command.hh"
#include "geo/point.hh"
#include "geo/intpoint.hh"

CommandContext::~CommandContext(){
}

Command::Command( CommandType type ) {
  m_type = type;
}

Command::~Command(){
}

void Command::DoRaster( CommandContext& context ){
  if ( m_type == CMD_TYPE_HYBRID ){
    assert( false ); // DoRaster must be overridden by hybrid commands
  }
  else if ( m_type == CMD_TYPE_OBJECT ){
    // Object commands have no raster steps
    return;
  }
  Do( context );
}

Command* Command::GetDWIM(){
  assert( false );
  return 0;
}

bool Command::HasDWIM() const{
  return false;
}

IntPoint Command::SelectionOffset() const{
  return IntPoint(0,0);
}

Point Command::Translate(const Point& p) const{
  return p;
}

CommandType Command::Type() const{
  return m_type;
}

void Command::Undo( CommandContext& ){
}

Operation::~Operation()
{}
