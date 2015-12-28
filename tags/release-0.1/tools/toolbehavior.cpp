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

#include "toolbehavior.hh"

const FaintSettings NullSettings;
NullNotifier nullNotifier;

ToolBehavior::ToolBehavior( ToolId id, SettingNotifier& notifier, bool eatSettings )
  : m_id(id),    
    m_settingEater( eatSettings ),
    m_relay( notifier )
{}

ToolBehavior::~ToolBehavior(){
}

ToolId ToolBehavior::GetId() const{
  return m_id;
}

ToolRefresh ToolBehavior::Char( wchar_t, int, int ){
  return TOOL_NONE;
}

ToolRefresh ToolBehavior::ToolSettingUpdate( const FaintSettings& s){
  if ( m_settings.Update(s) ){
    return TOOL_OVERLAY;
  }
  return TOOL_NONE;
}

bool ToolBehavior::HasSelection() const{
  return false;
}

ToolRefresh ToolBehavior::Deselect(){
  return TOOL_NONE;
}

void ToolBehavior::SelectionChange(){}

bool ToolBehavior::AcceptsPastedText(){
  return false;
}

bool ToolBehavior::CopyData( std::string&, bool ){
  return false;
}

bool ToolBehavior::CopyData( faint::Bitmap& ){
  return false;
}

ToolRefresh ToolBehavior::Delete(){
  return TOOL_NONE;
}

void ToolBehavior::Paste( const std::string& ){
}

ToolBehavior* ToolBehavior::GetNewTool(){
  return 0;
}

int ToolBehavior::GetCursor( const CursorPositionInfo& ){
  return 0;
}

int ToolBehavior::GetCursor( const CursorPositionInfo& info, int /* modifiers */ ){
  return GetCursor( info );
}

Command* ToolBehavior::GetCommand(){
  return 0;
}

unsigned int ToolBehavior::GetStatusFieldCount(){
    return 0;
}

const FaintSettings& ToolBehavior::GetSettings(){
  return m_settings;
}

bool ToolBehavior::EatsSettings(){
  return m_settingEater;
}

bool ToolBehavior::HasBitmap() const{
  return false;
}

faint::Bitmap* ToolBehavior::GetBitmap(){
  return 0;
}

void ToolBehavior::UpdateBitmap(){
}

ToolRefresh ToolBehavior::LeftDoubleClick( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

bool ToolBehavior::MouseOut(){
  return false;
}

