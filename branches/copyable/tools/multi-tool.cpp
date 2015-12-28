// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#include "tools/multi-tool.hh"

namespace faint{

MultiTool::MultiTool(ToolId id, default_task defaultTask, initial_task initialTask)
  : Tool(id),
    m_task(defaultTask, initialTask)
{}

bool MultiTool::AcceptsPastedText() const{
  return m_task->AcceptsPastedText();
}

bool MultiTool::CanUndo() const{
  return m_task->CanUndo();
}

bool MultiTool::CanRedo() const{
  return m_task->CanRedo();
}

ToolResult MultiTool::Char(const KeyInfo& info){
  return HandleTaskResult(m_task->Char(info));
}

bool MultiTool::CopyText(utf8_string& str, const erase_copied& copyMode){
  return m_task->CopyText(str, copyMode);
}

ToolResult MultiTool::Delete(){
  return HandleTaskResult(m_task->Delete());
}

ToolResult MultiTool::Deselect(){
  return HandleTaskResult(m_task->Deselect());
}

void MultiTool::Draw(FaintDC& dc, Overlays& overlays, const Point& pt){
  m_task->Draw(dc, overlays, pt);
}

bool MultiTool::DrawBeforeZoom(Layer layer) const{
  return m_task->DrawBeforeZoom(layer);
}

Command* MultiTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor MultiTool::GetCursor(const PosInfo& info) const{
  return m_task->GetCursor(info);
}

utf8_string MultiTool::GetRedoName() const{
  return m_task->GetRedoName();
}

IntRect MultiTool::GetRefreshRect(const IntRect& visible, const Point& currPos) const{
  return m_task->GetRefreshRect(visible, currPos);
}

utf8_string MultiTool::GetUndoName() const{
  return m_task->GetUndoName();
}

ToolResult MultiTool::DoubleClick(const PosInfo& info){
  return HandleTaskResult(m_task->DoubleClick(info));
}

ToolResult MultiTool::MouseDown(const PosInfo& info){
  return HandleTaskResult(m_task->MouseDown(info));
}

ToolResult MultiTool::MouseUp(const PosInfo& info){
  return HandleTaskResult(m_task->MouseUp(info));
}

ToolResult MultiTool::MouseMove(const PosInfo& info){
  return HandleTaskResult(m_task->MouseMove(info));
}

void MultiTool::Paste(const utf8_string& str){
  m_task->Paste(str);
}

ToolResult MultiTool::Preempt(const PosInfo& info){
  return HandleTaskResult(m_task->Preempt(info));
}

void MultiTool::Redo(){
  m_task->Redo();
}

ToolResult MultiTool::SelectAll(){
  return HandleTaskResult(m_task->SelectAll());
}

void MultiTool::SelectionChange(){
  m_task->SelectionChange();
}

bool MultiTool::SupportsSelection() const{
  return m_task->SupportsSelection();
}

void MultiTool::Undo(){
  m_task->Undo();
}

void MultiTool::UpdateTaskSettings(){
  m_task->UpdateSettings();
}

ToolResult MultiTool::HandleTaskResult(TaskResult r){
  if (r == TASK_DRAW){
    return TOOL_DRAW;
  }
  else if (r == TASK_CHANGE){
    if (m_task.Switch()){
      return TOOL_DRAW;
    }
    return TOOL_CHANGE;
  }
  else if (r == TASK_COMMIT){
    m_command.Set(m_task->GetCommand());
    return TOOL_COMMIT;
  }
  else if (r == TASK_COMMIT_AND_CHANGE){
    m_command.Set(m_task->GetCommand());
    if (m_task.Switch()){
      return TOOL_COMMIT;
    }
    return TOOL_CHANGE;
  }
  else if (r == TASK_PUSH){
    m_task.Push();
    return TOOL_DRAW;
  }
  return TOOL_NONE;
}

}
