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

#include "app/get-app-context.hh"
#include "commands/set-raster-selection-cmd.hh"
#include "tasks/raster-selection-base.hh"
#include "util/canvas.hh"
#include "util/raster-selection.hh"
#include "util/setting-util.hh"

namespace faint{

RasterSelectionTask::RasterSelectionTask()
{}

void RasterSelectionTask::UpdateSettings(){
  Canvas& canvas(GetCanvas());
  const RasterSelection& selection = canvas.GetRasterSelection();
  Command* cmd =
    set_selection_options_command(New(raster_selection_options(GetSettings())),
      Old(selection.GetOptions()));
  canvas.RunCommand(cmd);
}

}
