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

#ifndef FAINT_ORDER_OBJECT_CMD_HH
#define FAINT_ORDER_OBJECT_CMD_HH
#include "commands/command.hh"
#include "util/unique.hh"
class Object;

typedef Order<size_t>::New NewZ;
typedef Order<size_t>::Old OldZ;
bool forward( const NewZ&, const OldZ& );
std::string forward_or_back_str( const NewZ&, const OldZ& );

class OrderObjectCommand : public Command {
public:
  OrderObjectCommand(Object*, const NewZ&, const OldZ& );
  void Do( CommandContext& ) override;
  std::string Name() const override;
  void Undo( CommandContext& ) override;
private:
  Object* m_object;
  NewZ m_newZ;
  OldZ m_oldZ;
};

#endif
