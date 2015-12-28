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
#include "commands/remove-frame-cmd.hh"
#include "commands/reorder-frame-cmd.hh"

class RemoveFrame : public Command {
public:
  RemoveFrame(const index_t& index)
    : Command(CMD_TYPE_OBJECT),
      m_image(0),
      m_index(index)
  {}

  void Do(CommandContext& ctx){
    if ( m_image == 0 ){
      // Retrieve the image address on the first run for undo
      m_image = &ctx.GetFrame(m_index);
    }
    ctx.RemoveFrame(m_index);
  }

  void Undo(CommandContext& ctx){
    assert(m_image != 0);
    ctx.AddFrame(m_image, m_index);
  }

  std::string Name() const{
    return "Remove Frame";
  }
private:
  faint::Image* m_image;
  index_t m_index;
};

Command* remove_frame_command( const index_t& index ){
  return new RemoveFrame(index);
}

