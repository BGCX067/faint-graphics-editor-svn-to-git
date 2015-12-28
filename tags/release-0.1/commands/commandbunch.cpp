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

#include "commandbunch.hh"
#include <cassert>

CommandBunch::CommandBunch(CommandType type)
  : Command( type )
{}

CommandBunch::CommandBunch(CommandType type, const std::vector<Command*>& commands )
  : Command(type),
    m_commands( commands )
{}

CommandBunch::~CommandBunch(){
  for ( size_t i = 0; i!= m_commands.size(); i++ ){
    delete m_commands[i];
  }
}

void CommandBunch::Add(Command* cmd){
  m_commands.push_back(cmd);
}

void CommandBunch::DoRaster( faint::Image& img ){
  for ( size_t i = 0; i != m_commands.size(); i++ ){
    m_commands[i]->DoRaster( img );
  }
}

void CommandBunch::Do( faint::Image& img ){
  for ( size_t i = 0; i!= m_commands.size(); i++ ){
    m_commands[i]->Do( img );
  }
}

void CommandBunch::Undo( faint::Image& img ){
  size_t last = m_commands.size() - 1;
  for ( size_t i = 0; i != m_commands.size(); i++ ){
    m_commands[ last - i ]->Undo( img );
  }
}
