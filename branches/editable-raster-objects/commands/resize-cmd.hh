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

#ifndef FAINT_RESIZE_CMD_HH
#define FAINT_RESIZE_CMD_HH
#include "commands/command.hh"
#include "geo/geotypes.hh"
#include "util/unique.hh"

typedef Alternative<IntRect>::Type AltIntRect;
typedef Alternative<faint::DrawSource>::Type AltBgColor;

class ResizeCommand : public Command {
public:
  ResizeCommand( const IntRect&, const faint::DrawSource&, const std::string& name="Resize Image" );
  ResizeCommand( const IntRect&, const AltIntRect&, const faint::DrawSource&, const std::string& name="Resize Image" );
  ResizeCommand( const IntRect&, const faint::DrawSource&, const AltBgColor&, const std::string& name="Resize Image" );
  void Do( CommandContext& ) override;
  void DoRaster( CommandContext& ) override;
  Command* GetDWIM() override;
  bool HasDWIM() const override;
  std::string Name() const override;
  IntPoint SelectionOffset() const override;
  Point Translate(const Point&) const override;
  void Undo( CommandContext& ) override;
private:
  void SetDWIM();
  Optional<faint::DrawSource> m_altBg;
  Optional<IntRect> m_altRect;
  faint::DrawSource m_bg;
  bool m_isDwim;
  std::string m_name;
  IntRect m_rect;
};

#endif
