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

#ifndef FAINT_RASTERSELECTIONTASK_HH
#define FAINT_RASTERSELECTIONTASK_HH
#include "tasks/task.hh"

class RasterSelection;

class RasterSelectionTask : public Task {
  // Virtual base for common updating of the selection settings from
  // any raster selection task.
public:
  RasterSelectionTask(Settings&);
  bool UpdateSettings();
  void SetSelectionMasking( bool mask,
    const faint::Color&,
    bool alpha,
    RasterSelection& );
protected:
  Settings& m_settings;
private:
  RasterSelectionTask& operator=(const RasterSelectionTask&);
};

#endif
