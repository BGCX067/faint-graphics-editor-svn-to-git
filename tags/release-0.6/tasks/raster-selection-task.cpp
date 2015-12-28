#include "app/getappcontext.hh"
#include "commands/set-raster-selection.hh"
#include "tasks/raster-selection-task.hh"
#include "util/rasterselection.hh"
#include "util/settingutil.hh"

RasterSelectionTask::RasterSelectionTask(Settings& s)
  : m_settings(s)
{}

void RasterSelectionTask::SetSelectionMasking( bool mask, const faint::Color& color, bool alpha, RasterSelection& selection ){
  m_settings.Set( ts_BackgroundStyle, mask ? BackgroundStyle::MASKED :
    BackgroundStyle::SOLID );
  m_settings.Set( ts_BgCol, color );
  update_mask( mask, color, alpha, selection );
  GetAppContext().UpdateToolSettings(m_settings);
}

bool RasterSelectionTask::UpdateSettings(){
  const RasterSelection& selection = GetAppContext().GetActiveCanvas().GetRasterSelection(); // Demeter is spinning in her grave
  Command* cmd = new SetSelectionOptions( raster_selection_options(m_settings),
    selection.GetOptions());

  GetAppContext().GetActiveCanvas().RunCommand( cmd );
  return true; // Fixme
}

