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
#include "commands/command.hh"
#include "commands/set-pixel-cmd.hh"

class SetPixelCommand : public Command {
public:
  SetPixelCommand( const IntPoint& pos, const faint::Color& color )
    : Command( CommandType::RASTER ),
      m_color(color),
      m_pos(pos)
  {}

  void Do( CommandContext& context ) override{
    faint::put_pixel(context.GetRawBitmap(), m_pos, m_color);

  }
  std::string Name() const override{
    return "Set Pixel";
  }
private:
  faint::Color m_color;
  IntPoint m_pos;
};

Command* get_put_pixel_command( const IntPoint& pos, const faint::Color& color ){
  return new SetPixelCommand(pos, color);
}
