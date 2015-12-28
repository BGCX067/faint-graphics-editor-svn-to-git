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

#include "app/getappcontext.hh"
#include "tools/tool-wrapper.hh"

ToolWrapper::ToolWrapper(){
  m_switched = 0;
  m_lastSeen = 0;
}

bool ToolWrapper::DrawBeforeZoom() {
  return GetTool()->DrawBeforeZoom( GetAppContext().GetLayerType() );
}

Tool* ToolWrapper::GetTool(){
  AppContext& app = GetAppContext();
  if ( m_switched == 0 ){
    m_lastSeen = app.GetActiveTool();
    return m_lastSeen;
  }
  else {
    Tool* mainTool = app.GetActiveTool();
    if ( m_lastSeen != mainTool ){

      // The selected tool has been switched by the user since a tool
      // requested a switch. Lose the switch-info and revert to
      // using the common tool
      m_switched = 0;
      m_lastSeen = mainTool;
      return mainTool;
    }
    else {
      return m_switched;
    }
  }
}

const Tool& ToolWrapper::ToolRef() const{
  return *(const_cast<ToolWrapper*>(this)->GetTool());
}

ToolId ToolWrapper::GetToolId() const{
  return ToolRef().GetId();
}

void ToolWrapper::SetSwitched( Tool* tool ){
  AppContext& app = GetAppContext();
  Tool* mainTool = app.GetActiveTool();
  if ( mainTool == tool ){
    // Don't use the commonly selected tool as the switched tool,
    // just clear the switch info
    m_lastSeen = mainTool;
    m_switched = 0;
  }
  else {
    m_lastSeen = mainTool;
    m_switched = tool;
  }
}

void ToolWrapper::ClearSwitched(){
  delete m_switched;
  m_switched = 0;
  m_lastSeen = GetAppContext().GetActiveTool();
}
