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
#include "commands/command-bunch.hh"

class CommandBunch : public Command{
public:
  CommandBunch( CommandType type, const commands_t& commands, const bunch_name& name )
    : Command(type),
      m_commands(commands),
      m_name(name.Get())
  {
    assert(!commands.empty());
  }

  ~CommandBunch(){
    for ( size_t i = 0; i!= m_commands.size(); i++ ){
      delete m_commands[i];
    }
  }

  void Do( CommandContext& context ){
    for ( size_t i = 0; i!= m_commands.size(); i++ ){
      m_commands[i]->Do( context );
    }
  }

  void DoRaster( CommandContext& context ){
    for ( size_t i = 0; i != m_commands.size(); i++ ){
      m_commands[i]->DoRaster( context );
    }
  }

  std::string Name() const{
    return m_name.empty() ? m_commands.back()->Name() : m_name;
  }

  void Undo( CommandContext& context ){
    size_t last = m_commands.size() - 1;
    for ( size_t i = 0; i != m_commands.size(); i++ ){
      m_commands[ last - i ]->Undo( context );
    }
  }
private:
  std::vector<Command*> m_commands;
  std::string m_name;
};

Command* perhaps_bunch( CommandType type, const bunch_name& name, const commands_t& commands ){
  assert( !commands.empty() );
  return ( commands.size() == 1 ) ? commands.front() :
    new CommandBunch( type, commands, name );
}

Command* perhaps_bunch( CommandType type, const bunch_name& name, const std::deque<Command*>& commands ){
  assert( !commands.empty() );
  return ( commands.size() == 1 ) ? commands.front() :
    new CommandBunch( type, std::vector<Command*>( commands.begin(), commands.end() ), name );
}

Command* command_bunch( CommandType type, const bunch_name& name, const commands_t& commands ){
  return new CommandBunch( type, commands, name );
}

Command* command_bunch( CommandType type, const bunch_name& name, const std::deque<Command*>& commands ){
  return new CommandBunch( type,
    std::vector<Command*>( commands.begin(), commands.end() ),
    name );
}

Command* command_bunch( CommandType type, const bunch_name& name, Command* cmd1, Command* cmd2 ){
  std::vector<Command*> commands;
  commands.push_back(cmd1);
  commands.push_back(cmd2);
  return new CommandBunch( type, commands, name );
}
