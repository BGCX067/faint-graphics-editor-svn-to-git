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

#ifndef FAINT_POINT_CMD_HH
#define FAINT_POINT_CMD_HH
#include "command.hh"
#include "geo/point.hh"
#include "util/unique.hh"

typedef Order<Point>::New NewPoint;
typedef Order<Point>::Old OldPoint;

class PointCommand : public Command {
public:
  PointCommand( Object*, size_t pointIndex, const NewPoint&, const OldPoint& );
  std::string Name() const override;
  void Do( CommandContext& ) override;
  void Undo( CommandContext& ) override;
private:
  PointCommand& operator=(const PointCommand&);
  Object* m_object;
  size_t m_pointIndex;
  const Point m_new;
  const Point m_old;
};

#endif
