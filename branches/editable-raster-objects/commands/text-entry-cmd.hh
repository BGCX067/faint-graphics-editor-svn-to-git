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

#ifndef FAINT_TEXT_ENTRY_CMD_HH
#define FAINT_TEXT_ENTRY_CMD_HH
#include "command.hh"
#include "util/char.hh"
#include "util/unique.hh"
class ObjText;

typedef Order<faint::utf8_string>::New NewText;
typedef Order<faint::utf8_string>::Old OldText;

class TextEntryCommand : public Command{
public:
  TextEntryCommand( ObjText*, const NewText&, const OldText& );
  void Do( CommandContext& ) override;
  std::string Name() const override;
  void Undo( CommandContext& ) override;
private:
  TextEntryCommand& operator=(const TextEntryCommand&);
  ObjText* m_textObj;
  const faint::utf8_string m_old;
  const faint::utf8_string m_new;
};

#endif
