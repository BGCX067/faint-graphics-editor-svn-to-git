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

#ifndef FAINT_MULTITOOL_HH
#define FAINT_MULTITOOL_HH
#include "tasks/task.hh"

class MultiTool : public Tool {
  // A base for tools which forward most things to tasks.
public:
  MultiTool( ToolId, default_task, initial_task initial=initial_task(nullptr));
  MultiTool( ToolId, SettingNotifier&, default_task, initial_task initial=initial_task(nullptr));
  MultiTool( ToolId, SettingNotifier&, ToolSettingMode, default_task, initial_task initial=initial_task(nullptr));

  bool AcceptsPastedText() const override;
  bool CanRedo() const override;
  bool CanUndo() const override;
  ToolResult Char( const KeyInfo& ) override;
  bool CopyText( faint::utf8_string&, bool ) override;
  ToolResult Delete() override;
  ToolResult Deselect() override;
  void Draw( FaintDC&, Overlays&, const Point& ) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Cursor GetCursor( const CursorPositionInfo& ) const override;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const override;
  std::string GetRedoName() const override;
  std::string GetUndoName() const override;
  bool HasSelection() const override;
  ToolResult LeftDoubleClick( const CursorPositionInfo& ) override;
  ToolResult LeftDown( const CursorPositionInfo& ) override;
  ToolResult LeftUp( const CursorPositionInfo& ) override;
  ToolResult Motion( const CursorPositionInfo& ) override;
  void Paste( const faint::utf8_string& ) override;
  ToolResult Preempt( const CursorPositionInfo& ) override;
  void Redo() override;
  ToolResult SelectAll() override;
  void SelectionChange() override;
  void Undo() override;
protected:
  void ResetTask( default_task, initial_task initial=initial_task(nullptr) );
  void UpdateTaskSettings();
private:
  ToolResult HandleTaskResult(TaskResult);
  TaskWrapper m_task;
  PendingCommand m_command;
};

#endif
