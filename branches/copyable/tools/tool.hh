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

#ifndef FAINT_TOOL_HH
#define FAINT_TOOL_HH
#include "commands/command.hh"
#include "tools/tool-id.hh"
#include "util/common-fwd.hh"
#include "util/pos-info.hh"
#include "util/resource-id.hh"

namespace faint{

enum ToolResult{
  // Return values for Tool methods.
  TOOL_NONE, // Action had no externally relevant effect
  TOOL_DRAW, // Tool wants to draw
  TOOL_COMMIT, // Tool has a command ready
  TOOL_CHANGE, // Tool wishes to uhm switch to a new tool somehow
  TOOL_CANCEL // Tool aborted
};

class category_tool;
typedef Distinct<bool, category_tool, 0> erase_copied;

bool is_tool_modifier(int key);

class Tool {
public:
  explicit Tool(ToolId);
  virtual ~Tool();
  virtual bool AcceptsPastedText() const = 0;

  // Whether the tool currently allows redoing commands in the image.
  // Since tools can also feature undo/redo, it would be surprising
  // if the image-redo was triggered while an undo/redoable tool
  // was active (even when the tool doesn't have anything to redo).
  virtual bool AllowsGlobalRedo() const = 0;
  virtual bool CanRedo() const;
  virtual bool CanUndo() const;
  virtual ToolResult Char(const KeyInfo&);
  virtual bool CopyText(utf8_string& out, const erase_copied&);
  virtual ToolResult Delete();
  virtual ToolResult Deselect();
  virtual void Draw(FaintDC&, Overlays&, const Point& mousePos) = 0;

  // True if the tool should be rendered before scaling for zoom.
  // Typically raster tools need to be drawn before zoom is applied,
  // so that their output is scaled together with the background,
  // while vector tools should be drawn on top to appear smooth at all
  // zoom levels.
  virtual bool DrawBeforeZoom(Layer) const = 0;

  // True if the tool monopolizes tool-setting changes, or if it
  // allows changes to the common tool settings. In practice, probably
  // only used for making object selection setting changes affect the
  // objects, and leave the general tool settings unchanged.
  virtual bool EatsSettings() const = 0;
  virtual Command* GetCommand();
  virtual Cursor GetCursor(const PosInfo&) const = 0;
  ToolId GetId() const;

  // The (in image coordinates)-rectangle the tool has modified
  virtual IntRect GetRefreshRect(const IntRect& visible, const Point& mousePos) const = 0;
  virtual utf8_string GetRedoName() const;
  virtual const Settings& GetSettings() const = 0;
  virtual int GetStatusFieldCount() const;
  virtual utf8_string GetUndoName() const;

  virtual ToolResult DoubleClick(const PosInfo&);
  virtual ToolResult MouseDown(const PosInfo&) = 0;
  virtual ToolResult MouseUp(const PosInfo&) = 0;
  virtual ToolResult MouseMove(const PosInfo&) = 0;
  virtual void Paste(const utf8_string&);

  // The tool is given a chance to commit before being replaced
  virtual ToolResult Preempt(const PosInfo&) = 0;
  bool PreventsGlobalRedo() const;
  virtual void Redo();

  // Whether the tool requires a refresh when pointer leaves drawing
  // area. Not called during mouse capture, i.e. while mouse held.
  virtual bool RefreshOnMouseOut();
  virtual ToolResult SelectAll();

  // Called to notify that the selection changed outside the tool's
  // control.
  virtual void SelectionChange();

  // True if this tool has an internal selection concept, and
  // selection-related actions should be delegated to it.
  virtual bool SupportsSelection() const;

  // Call to update the settings in the tool. Returns true if any
  // setting used by the tool was changed.
  virtual bool UpdateSettings(const Settings&) = 0;

  virtual bool Set(const BoundSetting&) = 0;
  virtual void Undo();

  Tool& operator=(const Tool&) = delete;
private:
  ToolId m_id;
};

} // namespace

#endif
