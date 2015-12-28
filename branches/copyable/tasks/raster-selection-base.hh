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

#ifndef FAINT_RASTER_SELECTION_BASE_HH
#define FAINT_RASTER_SELECTION_BASE_HH
#include "tasks/task.hh"

class RasterSelection;

namespace faint{

class RasterSelectionTask : public Task {
  // Base for common updating of the selection settings from any
  // raster selection task.
public:
  RasterSelectionTask();
  void UpdateSettings() override;
  RasterSelectionTask& operator=(const RasterSelectionTask&) = delete;
private:
  virtual Canvas& GetCanvas() = 0;
  virtual const Settings& GetSettings() = 0;
};

}

#endif
