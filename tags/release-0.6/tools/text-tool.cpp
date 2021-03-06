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
#include "tasks/text-idle.hh"
#include "tools/text-tool.hh"
#include "util/settingutil.hh"

TextTool::TextTool()
  : MultiTool( T_TEXT, m_notifier, default_task(new TextIdle(m_settings)) )
{
  m_settings = default_text_settings();
  m_notifier.SetTarget( this );
}
