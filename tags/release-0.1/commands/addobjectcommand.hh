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

#ifndef FAINT_ADDOBJECTCOMMAND_HH
#define FAINT_ADDOBJECTCOMMAND_HH
#include "command.hh"
class Object;

class AddObjectCommand : public Command {
public:
  AddObjectCommand(Object*, bool select=false);
  ~AddObjectCommand();
  void Do( faint::Image& );
  void Undo( faint::Image& );
private:
  Object* m_object;
  bool m_select;
};

#endif
