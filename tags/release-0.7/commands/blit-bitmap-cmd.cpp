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

#include "commands/blit-bitmap-cmd.hh"
#include "rendering/faintdc.hh"
#include "util/settingutil.hh"

class BlitBitmapCommand : public Command {
public:
  BlitBitmapCommand( const IntPoint& pos, const faint::Bitmap& bmp )
    : Command( CMD_TYPE_RASTER ),
      m_bmp(bmp),
      m_pos(pos)
  {}

  void Do( CommandContext& context ){
    FaintDC& dc(context.GetDC());
    dc.Bitmap( m_bmp, floated(m_pos), default_bitmap_settings() );
  }

  std::string Name() const{
    return "Blit Bitmap";
  }
private:
  faint::Bitmap m_bmp;
  IntPoint m_pos;
};

Command* get_blit_bitmap_command( const IntPoint& pos, const faint::Bitmap& bmp ){
  return new BlitBitmapCommand(pos, bmp);
}
