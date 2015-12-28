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
#include "bitmap/bitmap.hh"
#include "commands/command-bunch.hh"
#include "commands/set-raster-selection-cmd.hh"
#include "tasks/move-selection.hh"
#include "tasks/select-raster-rectangle.hh"
#include "tasks/selection-idle.hh"
#include "tools/tool.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/util.hh"
#include "util/settingutil.hh"

class AppendSelection : public MergeCondition{
  // Used to add the selection to this command-bunch

public:
  AppendSelection()
    : m_appended(false)
  {}

  bool Satisfied(MergeCondition*) override{
    // AppendSelection is only used for appending,
    // not merging CommandBunches.
    return false;
  }

  bool Append(Command* cmd) override{
    if ( m_appended ){
      // Only append a single command
      return false;
    }
    m_appended = true;

    SetRasterSelection* candidate = dynamic_cast<SetRasterSelection*>(cmd);
    if ( candidate == nullptr || !candidate->ShouldAppend() ){
      return false;
    }
    return true;
  }

  bool AssumeName() const{
    // The rectangle selection command name should be used.
    return true;
  }
private:
  bool m_appended;
};

SelectionIdle::SelectionIdle(Settings& s)
  : RasterSelectionTask(s),
    m_fullRefresh(false)
{}

void SelectionIdle::Activate(){
  // Use full refresh to clear the single selection pixel left when
  // deselecting with SelectRectangle.
  m_fullRefresh = true;
}

void SelectionIdle::Draw( FaintDC&, Overlays&, const Point& ){
  m_fullRefresh = false;
}

bool SelectionIdle::DrawBeforeZoom( Layer ) const{
  return false;
}

Command* SelectionIdle::GetCommand(){
  return m_command.Retrieve();
}

Cursor SelectionIdle::GetCursor( const CursorPositionInfo& info ) const{
  return info.inSelection ? Cursor::MOVE : Cursor::CROSSHAIR;
}

Task* SelectionIdle::GetNewTask(){
  return m_newTask.Retrieve();
}

IntRect SelectionIdle::GetRefreshRect( const IntRect& visible, const Point& ) const{
  if ( m_fullRefresh ){
    return visible;
  }
  return IntRect::EmptyRect();
}

TaskResult SelectionIdle::LeftDown( const CursorPositionInfo& info ){
  const bool ctrlDown(fl(TOOLMODIFIER1, info.modifiers));

  if ( fl( RIGHT_MOUSE, info.modifiers ) ){
    const RasterSelection& selection = info.canvas->GetRasterSelection();
    if ( info.inSelection ){
      const IntPoint pos(floored(info.pos));
      IntPoint offset = pos - selection.TopLeft();
      if ( ctrlDown ){
        // Ctrl+Right-clicked point in selection, use clicked pixel
        // color as mask.
        RasterSelectionOptions newOptions(true,
          faint::DrawSource( selection.Floating() ?
            get_color(selection.GetBitmap(), offset) :
            get_color(info.canvas->GetBitmap(), pos)),
          m_settings.Get(ts_AlphaBlending));
        m_settings.Set(ts_BgCol, newOptions.bg);
        m_settings.Set(ts_BackgroundStyle, newOptions.mask ? BackgroundStyle::MASKED : BackgroundStyle::SOLID);
        m_command.Set(new SetSelectionOptions(New(newOptions),
            Old(selection.GetOptions())));
        GetAppContext().UpdateShownSettings();
        return TASK_COMMIT;
      }

      if ( selection.Floating() ) {
        // Lose the floating selection
        m_command.Set(selection.StampFloatingSelection());
      }
      // Fixme: ^^ The above commands will run first, then the
      // move-selection command - resulting in two commands.  They
      // should instead be bundled with the command created by the
      // MoveSelection-task later vvv
      // Right clicked in selection, move the selection rather than the
      // content
      m_newTask.Set( new MoveSelection(offset, selection.TopLeft(), copy_selected(false),
        false, // Don't float selected
        *info.canvas,
        m_settings ) );
      return m_command.Valid() ? TASK_COMMIT_AND_CHANGE : TASK_CHANGE;
    }

    if ( selection.Exists() ){
      if ( ctrlDown ){
        RasterSelectionOptions options(selection.GetOptions());
        options.mask = false;
        m_command.Set(new SetSelectionOptions(New(options), Old(selection.GetOptions())));
        return TASK_COMMIT;
      }
      // Right click outside: deselect
      m_command.Set(new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), "Deselect"));
      return TASK_COMMIT;
    }
    // Right click without selection doesn't mean anything
    return TASK_NONE;
  }

  if ( info.inSelection ){
    // Left click in selection: Move selected content
    const RasterSelection& selection = info.canvas->GetRasterSelection();
    if ( selection.Empty() ){
      return TASK_DRAW;
    }

    bool copying(fl(TOOLMODIFIER1, info.modifiers));

    if ( selection.Floating() && copying ){
      m_command.Set(selection.StampFloatingSelection());
    }
    IntPoint offset = floored( info.pos ) - selection.TopLeft();
    m_newTask.Set(new MoveSelection(offset, selection.TopLeft(), copy_selected(copying),
    true, // Float selected
    *info.canvas, m_settings));

    return copying ? TASK_COMMIT_AND_CHANGE :
      TASK_CHANGE;
  }

  // Left click outside, start new selection rectangle
  const RasterSelection& selection(info.canvas->GetRasterSelection());
  bool shouldMerge = !selection.Empty();
  m_newTask.Set(new SelectRectangle(info.pos, m_settings, shouldMerge));

  if ( !selection.Empty() ){
    // Deselection, and possibly stamping
    Command* deselect = new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), "Deselect Raster");

    if ( selection.Floating() ){
      Command* stamp = selection.StampFloatingSelection();
      m_command.Set(command_bunch( CommandType::HYBRID,
        bunch_name("Deselect Raster"),
        stamp, deselect, new AppendSelection()));
    }
    else{
      m_command.Set(command_bunch(CommandType::SELECTION,
        bunch_name("Deselect Raster"),
        deselect,
        new AppendSelection()));
    }
    return TASK_COMMIT_AND_CHANGE;
  }
  return TASK_CHANGE;
}

TaskResult SelectionIdle::LeftUp( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult SelectionIdle::Motion( const CursorPositionInfo& info ){
  if ( info.inSelection ){
    const RasterSelection& selection( info.canvas->GetRasterSelection() );
    if ( selection.Floating() ){
      info.status->SetMainText("Click to move the floating selection.");
    }
    else {
      info.status->SetMainText("Click to move the selected content.");
    }
  }
  else {
    info.status->SetMainText("Click to draw a selection rectangle.");
  }
  info.status->SetText( str(info.pos), 0 );
  return TASK_NONE;
}

TaskResult SelectionIdle::Preempt( const CursorPositionInfo& ){
  return TASK_NONE;
}

void SelectionIdle::SelectionChange(){
  AppContext& app(GetAppContext());
  m_settings = get_selection_settings(GetAppContext().GetActiveCanvas().GetRasterSelection());
  app.UpdateShownSettings();
}
