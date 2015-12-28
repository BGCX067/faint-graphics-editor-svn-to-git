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

#include "tools/tool.hh"
#include "util/keycode.hh"

namespace faint{

bool is_tool_modifier(int keycode){
  return keycode == key::ctrl || keycode == key::shift;
}

Tool::Tool(ToolId id)
  : m_id(id)
{}

Tool::~Tool(){
}

bool Tool::CanRedo() const{
  return false;
}

bool Tool::CanUndo() const{
  return false;
}

ToolResult Tool::Char(const KeyInfo&){
  return TOOL_NONE;
}

bool Tool::CopyText(utf8_string&, const erase_copied&){
  return false;
}

ToolResult Tool::Delete(){
  return TOOL_NONE;
}

ToolResult Tool::Deselect(){
  return TOOL_NONE;
}

Command* Tool::GetCommand(){
  return nullptr;
}

ToolId Tool::GetId() const{
  return m_id;
}

utf8_string Tool::GetRedoName() const{
  return "";
}

int Tool::GetStatusFieldCount() const{
  return 1;
}

utf8_string Tool::GetUndoName() const{
  return "";
}

bool Tool::SupportsSelection() const{
  return false;
}

ToolResult Tool::DoubleClick(const PosInfo& info){
  // Default to forwarding the double click as another left down
  return MouseDown(info);
}

void Tool::Paste(const utf8_string&){
  assert(false);
}

bool Tool::PreventsGlobalRedo() const{
  return !AllowsGlobalRedo();
}

void Tool::Redo(){
  assert(false);
}

bool Tool::RefreshOnMouseOut(){
  return false;
}

ToolResult Tool::SelectAll(){
  return TOOL_NONE;
}

void Tool::SelectionChange(){
}

void Tool::Undo(){
  assert(false);
}

} // namespace
