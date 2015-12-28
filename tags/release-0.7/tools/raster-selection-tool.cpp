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
#include "tasks/selection-idle.hh"
#include "tools/raster-selection-tool.hh"
#include "util/settingutil.hh"

RectangleSelectTool::RectangleSelectTool() :
  MultiTool( T_RECT_SEL, m_notifier,
    EAT_SETTINGS,
    default_task(new SelectionIdle(m_settings)))
{
  m_settings = get_selection_settings(GetAppContext().GetActiveCanvas().GetRasterSelection());
  m_notifier.SetTarget(this);
}
