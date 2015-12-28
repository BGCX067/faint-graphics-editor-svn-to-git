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

#ifndef FAINT_OBJECT_SELECTION_TOOL_HH
#define FAINT_OBJECT_SELECTION_TOOL_HH
#include "tools/multi-tool.hh"

class AppContext;
class ObjSettingNotifier : public SettingNotifier {
public:
  void Notify( const BoolSetting&, bool ) override;
  void Notify( const IntSetting&, int ) override;
  void Notify( const StrSetting&, const std::string& ) override;
  void Notify( const ColorSetting&, const faint::DrawSource& ) override;
  void Notify( const FloatSetting&, FloatSetting::ValueType ) override;
private:
  void RunCommand( Command* );
};

class ObjSelectTool : public MultiTool {
public:
  ObjSelectTool();
private:
  ObjSettingNotifier m_notifier;
};

#endif
