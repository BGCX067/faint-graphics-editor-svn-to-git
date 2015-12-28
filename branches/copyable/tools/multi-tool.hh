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

#ifndef FAINT_MULTI_TOOL_HH
#define FAINT_MULTI_TOOL_HH
#include "tasks/task.hh"

namespace faint{

class MultiTool : public Tool {
  // A base for tools which forward most things to tasks.
public:
  MultiTool(ToolId, default_task, initial_task initial=initial_task(nullptr));

  bool AcceptsPastedText() const override;
  bool CanRedo() const override;
  bool CanUndo() const override;
  ToolResult Char(const KeyInfo&) override;
  bool CopyText(utf8_string&, const erase_copied&) override;
  ToolResult Delete() override;
  ToolResult Deselect() override;
  void Draw(FaintDC&, Overlays&, const Point&) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Cursor GetCursor(const PosInfo&) const override;
  IntRect GetRefreshRect(const IntRect&, const Point&) const override;
  utf8_string GetRedoName() const override;
  utf8_string GetUndoName() const override;
  ToolResult DoubleClick(const PosInfo&) override;
  ToolResult MouseDown(const PosInfo&) override;
  ToolResult MouseUp(const PosInfo&) override;
  ToolResult MouseMove(const PosInfo&) override;
  void Paste(const utf8_string&) override;
  ToolResult Preempt(const PosInfo&) override;
  void Redo() override;
  ToolResult SelectAll() override;
  void SelectionChange() override;
  bool SupportsSelection() const override;
  void Undo() override;
protected:
  void UpdateTaskSettings();
private:
  ToolResult HandleTaskResult(TaskResult);
  Settings m_settings;
  TaskWrapper m_task;
  PendingCommand m_command;
};

}

#endif
