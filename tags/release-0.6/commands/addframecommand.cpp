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

#include "commands/addframecommand.hh"
#include "util/image.hh"

AddFrameCommand::AddFrameCommand(const IntSize& size)
  : Command(CMD_TYPE_OBJECT) // Fixme: Add command type?
{
  ImageInfo info(size);
  ImageProps props(info);
  m_image = new faint::Image(props);
}

AddFrameCommand::AddFrameCommand(faint::Image* frame, size_t index)
  : Command(CMD_TYPE_OBJECT), // Fixme: Add command type?
    m_image(frame),
    m_index(index)
{}

AddFrameCommand::~AddFrameCommand(){
  delete m_image;
}

void AddFrameCommand::Do(CommandContext& context){
  if ( m_index.IsSet() ){
    context.AddFrame(m_image, m_index.Get());
  }
  else{
    context.AddFrame(m_image);
  }
}

void AddFrameCommand::Undo(CommandContext& context){
  context.RemoveFrame(m_image);
}

std::string AddFrameCommand::Name() const{
  return "Add Frame";
}
