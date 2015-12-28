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

#ifndef FAINT_RESIZECOMMAND_HH
#define FAINT_RESIZECOMMAND_HH
#include "commands/command.hh"
#include "geo/geotypes.hh"
#include "util/unique.hh"

typedef Alternative<IntRect>::Type AltIntRect;
typedef Alternative<faint::Color>::Type AltBgColor;

class ResizeCommand : public Command {
public:
  ResizeCommand( const IntRect&, const faint::Color& bgColor, const std::string& name="Resize Image" );
  ResizeCommand( const IntRect&, const AltIntRect&, const faint::Color& bgColor, const std::string& name="Resize Image" );
  ResizeCommand( const IntRect&, const faint::Color&, const AltBgColor&, const std::string& name="Resize Image" );
  void Do( CommandContext& );
  void DoRaster( CommandContext& );
  Command* GetDWIM();
  bool HasDWIM() const;
  std::string Name() const;
  IntPoint SelectionOffset() const;
  Point Translate(const Point&) const;
  void Undo( CommandContext& );
private:
  void SetDWIM();
  Optional<faint::Color> m_alternateColor;
  Optional<IntRect> m_alternateRect;
  faint::Color m_bgColor;
  bool m_isDwim;
  std::string m_name;
  IntRect m_rect;
};

#endif
