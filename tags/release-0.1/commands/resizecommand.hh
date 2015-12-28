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
#include "command.hh"
#include "geo/geotypes.hh"

class ResizeCommand : public Command {
public:
  ResizeCommand( const IntSize&, const faint::Color& bgColor );
  ResizeCommand( const IntRect&, const faint::Color& bgColor );
  void Do( faint::Image& );
  void Undo( faint::Image& );
  void DoRaster( faint::Image& );
private:
  IntRect m_rect;
  faint::Color m_bgColor;
};

#endif
