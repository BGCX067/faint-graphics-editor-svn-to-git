// -*- coding: us-ascii-unix -*-
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

#include <cassert>
#include "commands/command.hh"
#include "geo/intpoint.hh"
#include "geo/point.hh"
#include "rendering/faint-dc.hh"
#include "util/setting-util.hh"

namespace faint{

bool fully_reversible(CommandType t){
  return !has_raster_steps(t);
}

bool has_raster_steps(CommandType t){
  return t == CommandType::RASTER || t == CommandType::HYBRID;
}

bool somewhat_reversible(CommandType t){
  return t != CommandType::RASTER;
}

CommandContext::~CommandContext(){
}

Command::Command(CommandType type) {
  m_type = type;
}

Command::~Command(){
}

void Command::DoRaster(CommandContext& context){
  if (m_type == CommandType::HYBRID){
    assert(false); // DoRaster must be overridden by hybrid commands
  }
  else if (!has_raster_steps(m_type)){
    return;
  }
  Do(context);
}

Command* Command::GetDWIM(){
  assert(false);
  return nullptr;
}

CommandId Command::GetId() const{
  return m_id;
}

bool Command::HasDWIM() const{
  return false;
}

bool Command::Merge(Command*, bool){
  return false;
}

bool Command::ModifiesState() const{
  return true;
}

IntPoint Command::SelectionOffset() const{
  return IntPoint(0,0);
}

Point Command::Translate(const Point& p) const{
  return p;
}

CommandType Command::Type() const{
  return m_type;
}

void Command::Undo(CommandContext&){
}

Point Command::UndoTranslate(const Point& p) const{
  return p;
}

OldCommand::OldCommand(UndoType in_type)
  : command(nullptr),
    targetFrame(nullptr),
    type(in_type)
{
  assert(type != UndoType::NORMAL_COMMAND);
}

OldCommand::OldCommand(Command* in_command, Image* in_targetFrame)
  : command(in_command),
    targetFrame(in_targetFrame),
    type(UndoType::NORMAL_COMMAND)
{
  assert(command != nullptr);
  assert(targetFrame != nullptr);
}

OldCommand OldCommand::OpenGroup(){
  return OldCommand(UndoType::OPEN_GROUP);
}

OldCommand OldCommand::CloseGroup(){
  return OldCommand(UndoType::CLOSE_GROUP);
}

bool OldCommand::Merge(OldCommand& candidate){
  return type == UndoType::NORMAL_COMMAND &&
    candidate.type == UndoType::NORMAL_COMMAND &&
    command->Merge(candidate.command,
    targetFrame == candidate.targetFrame);
}

Operation::~Operation()
{}

}
