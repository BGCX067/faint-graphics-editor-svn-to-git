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
#include "commands/reorder-frame-cmd.hh"

class ReorderFrame : public Command {
public:
  ReorderFrame(const new_index_t& newIndex, const old_index_t& oldIndex)
    : Command(CMD_TYPE_OBJECT),
      m_newIndex(newIndex),
      m_oldIndex(oldIndex)
  {}

  void Do(CommandContext& ctx){
    ctx.ReorderFrame(m_newIndex, m_oldIndex);
  }

  void Undo(CommandContext& ctx){
    if ( m_oldIndex.Get() < m_newIndex.Get() ){
      ctx.ReorderFrame(New(m_oldIndex.Get()), Old(m_newIndex.Get() - 1));
    }
    else {
      ctx.ReorderFrame(New(m_oldIndex.Get()), Old(m_newIndex.Get()));
    }
  }

  std::string Name() const{
    return "Reorder Frames";
  }
private:
  new_index_t m_newIndex;
  old_index_t m_oldIndex;
};

Command* reorder_frame_command( const new_index_t& newIndex, const old_index_t& oldIndex ){
  return new ReorderFrame(newIndex, oldIndex);
}
