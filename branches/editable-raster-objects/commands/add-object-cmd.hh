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

#ifndef FAINT_ADD_OBJECT_CMD_HH
#define FAINT_ADD_OBJECT_CMD_HH
#include "command.hh"
#include "util/unique.hh"
class Object;

class AddObjectCommand : public Command {
public:
  AddObjectCommand(Object*, const select_added&, const std::string& name="Add" );
  ~AddObjectCommand();
  void Do( CommandContext& ) override;
  void Undo( CommandContext& ) override;
  std::string Name() const override;
private:
  Object* m_object;
  bool m_select;
  std::string m_name;
};

#endif
