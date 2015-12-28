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

#ifndef FAINT_DELOBJECTCOMMAND_HH
#define FAINT_DELOBJECTCOMMAND_HH
#include "command.hh"
#include "object.hh"

class DelObjectCommand : public Command {
public:  
  DelObjectCommand(Object*, int z);
  virtual void Do( faint::Image& );
  virtual void Undo( faint::Image& );
private:
  Object* m_object;
  int m_pos;
};

#endif
