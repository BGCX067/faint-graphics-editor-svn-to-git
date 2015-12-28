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
#include "commands/set-raster-selection-cmd.hh"
#include "tasks/raster-selection-task.hh"
#include "util/rasterselection.hh"
#include "util/settingutil.hh"

RasterSelectionTask::RasterSelectionTask(Settings& s)
  : m_settings(s)
{}

void RasterSelectionTask::SetSelectionMasking( bool enableMask, const faint::DrawSource& bg, bool enableAlpha, RasterSelection& selection ){
  m_settings.Set( ts_BackgroundStyle, enableMask ? BackgroundStyle::MASKED :
    BackgroundStyle::SOLID );
  m_settings.Set( ts_BgCol, bg );
  update_mask( enableMask, bg, enableAlpha, selection );
  GetAppContext().UpdateToolSettings(m_settings);
}

bool RasterSelectionTask::UpdateSettings(){
  const RasterSelection& selection = GetAppContext().GetActiveCanvas().GetRasterSelection(); // Demeter is spinning in her grave
  Command* cmd = new SetSelectionOptions( New(raster_selection_options(m_settings)),
    Old(selection.GetOptions()) );

  GetAppContext().GetActiveCanvas().RunCommand( cmd );
  return true; // Fixme
}
