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
#include "commands/set-frame-delay.hh"

class SetFrameDelay : public Command {
public:
  SetFrameDelay(size_t frameIndex, const NewDelay& newDelay, const OldDelay& oldDelay)
    : Command(CMD_TYPE_OBJECT),
      m_frameIndex(frameIndex),
      m_oldDelay(oldDelay.Get()),
      m_newDelay(newDelay.Get())
  {}

  void Do(CommandContext& ctx){
    ctx.SetFrameDelay(m_frameIndex, m_newDelay);
  }

  void Undo(CommandContext& ctx){
    ctx.SetFrameDelay(m_frameIndex, m_oldDelay);
  }

  std::string Name() const{
    return "Set Frame Delay";
  }
private:
  size_t m_frameIndex;
  delay_t m_newDelay;
  delay_t m_oldDelay;
};

Command* set_frame_delay_command( size_t frameIndex, const NewDelay& newDelay, const OldDelay& oldDelay ){
  return new SetFrameDelay(frameIndex, newDelay, oldDelay);
}
