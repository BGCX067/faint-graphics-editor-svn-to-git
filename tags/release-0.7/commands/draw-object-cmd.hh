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

#ifndef FAINT_DRAW_OBJECT_CMD_HH
#define FAINT_DRAW_OBJECT_CMD_HH
#include "commands/command.hh"
#include "util/unique.hh"
class Object;

class DrawObjectCommand : public Command{
  // Draws the object (as raster graphics).  Can assume ownership (the
  // its_yours-constructor), and will then delete the object on
  // destruction.
public:
  DrawObjectCommand( const its_yours_t<Object>& );
  DrawObjectCommand( const just_a_loan_t<Object>& );
  ~DrawObjectCommand();
  void Do( CommandContext& );
  std::string Name() const;
private:
  bool m_delete;
  Object* m_object;
};

#endif
