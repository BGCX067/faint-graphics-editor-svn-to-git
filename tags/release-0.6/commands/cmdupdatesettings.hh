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

#ifndef FAINT_CMDUPDATESETTINGS_HH
#define FAINT_CMDUPDATESETTINGS_HH
#include "commands/command.hh"
#include "objects/object.hh"
#include "util/unique.hh"

typedef Order<Settings>::New NewSettings;
typedef Order<Settings>::Old OldSettings;

class CmdUpdateSettings : public Command {
public:
  CmdUpdateSettings( Object*, const NewSettings&, const OldSettings& );
  void Do( CommandContext& );
  std::string Name() const;
  void Undo( CommandContext& );
private:
  Settings m_newSettings;
  Object* m_object;
  Settings m_oldSettings;

};

#endif
