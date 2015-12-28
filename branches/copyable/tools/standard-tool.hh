// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_STANDARD_TOOL_HH
#define FAINT_STANDARD_TOOL_HH
#include "tools/tool.hh"

namespace faint{

bool sub_pixel(const PosInfo&);

class StandardTool : public Tool{
  // Base class for tools with some default behavior and settings
  // management. Can not be used as a base for tools which need special
  // actions when settings change.
public:
  StandardTool(ToolId, const Settings&);
  bool AcceptsPastedText() const override;
  bool AllowsGlobalRedo() const override;
  bool EatsSettings() const override;
  const Settings& GetSettings() const override;
  bool Set(const BoundSetting&) override;
  bool UpdateSettings(const Settings&) override;
protected:
  bool GetAntiAlias() const;
  bool SetAntiAlias(const PosInfo&);
  void SetSwapColors(bool);
private:
  Settings m_settings;
};

} // namespace

#endif
