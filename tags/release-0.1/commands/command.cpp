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
Command::Command( CommandType type ) {
  m_type = type;
}

Command::~Command(){
}

void Command::DoRaster( faint::Image& image ){
  if ( m_type == CMD_TYPE_HYBRID ){
    // DoRaster must be overridden by hybrid commands
    assert( false );
  }
  else if ( m_type == CMD_TYPE_OBJECT ){
    // Object commands have no raster steps
    return;
  }

  Do( image );
}

void Command::Undo( faint::Image& ){
}

CommandType Command::GetType() {
  return m_type;
}

PendingCommand::PendingCommand()
  : m_command(0)
{}

PendingCommand::~PendingCommand(){
  delete m_command;
}

void PendingCommand::Set( Command* cmd ){
  delete m_command;
  m_command = cmd;
}

Command* PendingCommand::Retrieve(){
  Command* command = m_command;
  m_command = 0;
  return command;
}
