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

Tool::Tool( ToolId id, SettingNotifier& notifier, bool eatSettings )
  : m_id(id),
    m_relay( notifier ),
    m_settingEater( eatSettings )
{}

Tool::~Tool(){
}

bool Tool::AcceptsPastedText() const{
  return false;
}

bool Tool::AllowsGlobalRedo() const{
  return true;
}

bool Tool::CanRedo() const{
  return false;
}

bool Tool::CanUndo() const{
  return false;
}

ToolResult Tool::Char( const KeyInfo& ){
  return TOOL_NONE;
}

bool Tool::CopyText( faint::utf8_string&, bool ){
  return false;
}

ToolResult Tool::Delete(){
  return TOOL_NONE;
}

ToolResult Tool::Deselect(){
  return TOOL_NONE;
}

bool Tool::EatsSettings() const{
  return m_settingEater;
}

Command* Tool::GetCommand(){
  return 0;
}

ToolId Tool::GetId() const{
  return m_id;
}

std::string Tool::GetRedoName() const{
  return "";
}

const Settings& Tool::GetSettings(){
  return m_settings;
}

unsigned int Tool::GetStatusFieldCount() const{
  return 1;
}

std::string Tool::GetUndoName() const{
  return "";
}

bool Tool::HasSelection() const{
  return false;
}

ToolResult Tool::LeftDoubleClick( const CursorPositionInfo& ){
  return TOOL_NONE;
}

void Tool::Paste( const faint::utf8_string& ){
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

ToolResult Tool::ToolSettingUpdate( const Settings& s){
  if ( m_settings.Update(s) ){
    return TOOL_DRAW;
  }
  return TOOL_NONE;
}

void Tool::Undo(){
  assert(false);
}
