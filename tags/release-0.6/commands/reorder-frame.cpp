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
#include "commands/command.hh"
#include "commands/reorder-frame.hh"

class ReorderFrame : public Command {
public:
  ReorderFrame(size_t newIndex, size_t oldIndex)
    : Command(CMD_TYPE_OBJECT),
      m_newIndex(newIndex),
      m_oldIndex(oldIndex)
  {}

  void Do(CommandContext& ctx){
    ctx.ReorderFrame(m_newIndex, m_oldIndex);
  }

  void Undo(CommandContext& ctx){
    if ( m_oldIndex < m_newIndex ){
      ctx.ReorderFrame(m_oldIndex, m_newIndex - 1);
    }
    else {
      ctx.ReorderFrame(m_oldIndex, m_newIndex);
    }

  }

  std::string Name() const{
    return "Reorder Frames";
  }
private:
  size_t m_newIndex;
  size_t m_oldIndex;
};

Command* reorder_frame_command( size_t newIndex, size_t oldIndex ){
  return new ReorderFrame(newIndex, oldIndex);
}
