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

#ifndef FAINT_TOOLWRAPPER_HH
#define FAINT_TOOLWRAPPER_HH
#include "settingid.hh"
#include "toolbehavior.hh"

// This class is a terrible thing I invented to get a little more
// control over the in-image tool switching which happens when
// for example (and maybe only) a text object is double-clicked with
// the selection tool.
//
// This fights furiously with the application-wide tool-choice. :(
class ToolWrapper{
public:
  ToolWrapper();
  ToolBehavior* Tool();
  ToolId GetToolId();  
  bool DrawBeforeZoom();
  void SetSwitched( ToolBehavior* );
private:
  ToolBehavior* m_switched;
  ToolBehavior* m_lastSeen;
};

#endif
