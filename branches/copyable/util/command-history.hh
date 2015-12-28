// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_COMMAND_HISTORY_HH
#define FAINT_COMMAND_HISTORY_HH
#include <deque>
#include "commands/command.hh"
#include "geo/canvas-geo.hh"
#include "gui/canvas-panel-contexts.hh"
#include "tools/tool-wrapper.hh"
#include "util/template-fwd.hh"

class category_apply_command;
typedef Distinct<bool, category_apply_command, 0> clear_redo;

namespace faint{
class ImageList;

class CommandHistory{
public:
  CommandHistory();
  ~CommandHistory();

  // Applies the specified command. Returns the image offset(?), if any.
  Optional<IntPoint> Apply(Command*, const clear_redo&, Image*, ImageList&, TargetableCommandContext&, const CanvasGeo&);
  bool ApplyDWIM(ImageList&, TargetableCommandContext&, const CanvasGeo&);
  bool Bundling() const;

  bool CanRedo() const;
  bool CanUndo() const;
  void CloseUndoBundle();

  faint::utf8_string GetRedoName(const ImageList&) const;
  faint::utf8_string GetUndoName(const ImageList&) const;

  void OpenUndoBundle();

  // Returns the latest command that modified the image state (or an
  // unset Optional if there's no modifying command in the list)
  Optional<CommandId> GetLastModifying() const;

  void Redo(TargetableCommandContext&, const CanvasGeo&, faint::ImageList&);
  bool Undo(TargetableCommandContext&, const CanvasGeo&);
private:
  std::deque<OldCommand> m_undoList;
  std::deque<OldCommand> m_redoList;
  bool m_openBundle;
};

} // namespace

#endif
