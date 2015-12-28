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

#ifndef FAINT_RESCALE_CMD_HH
#define FAINT_RESCALE_CMD_HH
#include "commands/command.hh"
#include "geo/intsize.hh"

class RescaleCommand : public Command {
public:
  RescaleCommand( const IntSize&, ScaleQuality );
  ~RescaleCommand();
  void Do( CommandContext& ) override;
  void DoRaster( CommandContext& ) override;
  std::string Name() const override;
  Point Translate( const Point& ) const override;
  void Undo( CommandContext& ) override;
private:
  IntSize m_size;
  IntSize m_oldSize;
  ScaleQuality m_quality;
  Command* m_objectResize;
};

#endif
