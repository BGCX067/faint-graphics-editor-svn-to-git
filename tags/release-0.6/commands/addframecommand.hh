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

#ifndef FAINT_ADD_FRAME_COMMAND_HH
#define FAINT_ADD_FRAME_COMMAND_HH
#include "commands/command.hh"
#include "util/commonfwd.hh"
#include "util/unique.hh"

class AddFrameCommand : public Command {
public:
  AddFrameCommand(const IntSize&);
  AddFrameCommand(faint::Image*, size_t index);
  ~AddFrameCommand();
  void Do(CommandContext&);
  void Undo( CommandContext& );
  std::string Name() const;
private:
  faint::Image* m_image;
  Optional<size_t> m_index;
};

#endif
