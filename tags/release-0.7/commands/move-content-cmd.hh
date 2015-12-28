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

#ifndef FAINT_MOVE_CONTENT_CMD_HH
#define FAINT_MOVE_CONTENT_CMD_HH
#include "bitmap/bitmap.hh"
#include "commands/command.hh"
#include "util/unique.hh"

class CmdMoveContent;
typedef Unique<bool, CmdMoveContent, 0> copy_content;
typedef Order<IntRect>::New NewIntRect;
typedef Order<IntRect>::Old OldIntRect;

class CmdMoveContent : public Command { // Fixme: Deprecated command?
public:
  CmdMoveContent( const faint::Bitmap&, const NewIntRect&, const OldIntRect&,
    const copy_content&, const Settings&, const std::string& name );
  void Do( CommandContext& );
  std::string Name() const;
private:
  faint::Bitmap m_bitmap;
  bool m_copy;
  IntRect m_oldRect;
  IntRect m_rect;
  Settings m_settings;
  std::string m_name;
};

#endif
