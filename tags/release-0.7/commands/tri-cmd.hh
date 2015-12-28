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

#ifndef FAINT_TRI_CMD_HH
#define FAINT_TRI_CMD_HH
#include "commands/command.hh"
#include "geo/tri.hh"
#include "util/unique.hh"
class Object;

typedef Order<Tri>::New NewTri;
typedef Order<Tri>::Old OldTri;

class TriCommand : public Command {
public:
  TriCommand( Object*, const NewTri&, const OldTri&, const std::string& name="Adjust" );
  void Do( CommandContext& );
  std::string Name() const;
  void Undo( CommandContext& );
private:
  TriCommand& operator=( const TriCommand& );
  Object* m_object;
  const Tri m_new;
  const Tri m_old;
  std::string m_name;
};

#endif
