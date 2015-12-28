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
  MultiTool( ToolId, default_task, initial_task initial=initial_task(0));
  MultiTool( ToolId, SettingNotifier&, default_task, initial_task initial=initial_task(0));
  MultiTool( ToolId, SettingNotifier&, bool eatsSettings, default_task, initial_task initial=initial_task(0));
  bool AcceptsPastedText() const;
  bool CanRedo() const;
  bool CanUndo() const;
  ToolResult Char( const KeyInfo& );
  bool CopyText( faint::utf8_string&, bool );
  ToolResult Delete();
  ToolResult Deselect();
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Cursor::type GetCursor( const CursorPositionInfo& ) const;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  std::string GetRedoName() const;
  std::string GetUndoName() const;
  bool HasSelection() const;
  ToolResult LeftDoubleClick( const CursorPositionInfo& );
  ToolResult LeftDown( const CursorPositionInfo& );
  ToolResult LeftUp( const CursorPositionInfo& );
  ToolResult Motion( const CursorPositionInfo& );
  void Paste( const faint::utf8_string& );
  ToolResult Preempt( const CursorPositionInfo& );
  void Redo();
  ToolResult SelectAll();
  void SelectionChange();
  void Undo();
protected:
  void ResetTask( default_task, initial_task initial=initial_task(0) );
  void UpdateTaskSettings();
private:
  ToolResult HandleTaskResult(TaskResult);
  TaskWrapper m_task;
  PendingCommand m_command;
};

#endif
