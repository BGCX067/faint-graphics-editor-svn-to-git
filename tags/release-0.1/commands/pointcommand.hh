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

#ifndef FAINT_POINTCOMMAND_HH
#define FAINT_POINTCOMMAND_HH
#include "command.hh"

class Object;
class PointCommand : public Command {
public:
  PointCommand( Object*, size_t pointIndex, const Point& newPoint, const Point& oldPoint );
  void Do( faint::Image& );
  void Undo( faint::Image& );
private:
  PointCommand& operator=(const PointCommand&);
  Object* m_object;
  size_t m_pointIndex;
  const Point m_new;
  const Point m_old;
};

#endif
