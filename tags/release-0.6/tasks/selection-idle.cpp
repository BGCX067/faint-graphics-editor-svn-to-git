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
#include "commands/commandbunch.hh"
#include "commands/set-raster-selection.hh"
#include "tasks/move-selection.hh"
#include "tasks/select-raster-rectangle.hh"
#include "tasks/selection-idle.hh"
#include "tools/tool.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/util.hh"
#include "util/settingutil.hh"

SelectionIdle::SelectionIdle(Settings& s)
  : RasterSelectionTask(s),
    m_fullRefresh(false)
{}

bool SelectionIdle::Draw( FaintDC&, Overlays&, const Point& ){
  m_fullRefresh = false;
  return false;
}

bool SelectionIdle::DrawBeforeZoom( Layer::type ) const{
  return false;
}

Command* SelectionIdle::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type SelectionIdle::GetCursor( const CursorPositionInfo& info ) const{
  return info.inSelection ? Cursor::MOVE : Cursor::CROSSHAIR;
}

Task* SelectionIdle::GetNewTask(){
  return m_newTask.Retrieve();
}

IntRect SelectionIdle::GetRefreshRect( const IntRect& visible, const Point& ) const{
  if ( m_fullRefresh ){
    // Full refresh is used when mask settings change, so that the
    // void (if any) as well as the selection is redrawn.
    return visible;
  }
  return IntRect::EmptyRect();
}

TaskResult SelectionIdle::LeftDown( const CursorPositionInfo& info ){
  const bool ctrlDown(fl(TOOLMODIFIER1, info.modifiers));

  if ( fl( RIGHT_MOUSE, info.modifiers ) ){
    const RasterSelection& selection = info.canvas->GetRasterSelection();
    if ( info.inSelection ){
      IntPoint offset = truncated( info.pos ) - selection.TopLeft();
      if ( ctrlDown ){
	// Ctrl+Right-clicked point in selection, use clicked pixel
	// color as mask.
	RasterSelectionOptions newOptions(true,
	  selection.Floating() ? 
	  get_color(selection.GetBitmap(), offset) :
	  get_color(info.canvas->GetBitmap(), truncated(info.pos)),
	  m_settings.Get(ts_AlphaBlending));
	m_settings.Set(ts_BgCol, newOptions.bgCol);
	m_settings.Set(ts_BackgroundStyle, newOptions.mask ? BackgroundStyle::MASKED : BackgroundStyle::SOLID);
	m_command.Set(new SetSelectionOptions(newOptions,
            selection.GetOptions()));
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
        m_command.Set(new SetSelectionOptions(options, selection.GetOptions()));
        return TASK_COMMIT;
      }
      // Right click outside: deselect
      m_command.Set(new SetRasterSelection(RasterSelectionState(), selection.GetState(), "Deselect"));
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
    IntPoint offset = truncated( info.pos ) - selection.TopLeft();
    m_newTask.Set(new MoveSelection(offset, selection.TopLeft(), copy_selected(copying),
    true, // Float selected
    *info.canvas, m_settings));

    return copying ? TASK_COMMIT_AND_CHANGE :
      TASK_CHANGE;
  }

  // Left click outside, start new selection rectangle
  const RasterSelection& selection(info.canvas->GetRasterSelection());
  m_newTask.Set(new SelectRectangle(info.pos, selection.GetState(), m_settings )); // Why old state passed?

  if ( !selection.Empty() ){
    // Deselection, and possibly stamping
    Command* deselect = new SetRasterSelection(RasterSelectionState(), selection.GetState(), "Deselect Raster");
    m_command.Set( selection.Floating() ?
      command_bunch( CMD_TYPE_HYBRID, selection.StampFloatingSelection(), deselect, bunch_name("Deselect Raster")) : // Fixme: CMD_TYPE
      deselect);
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
