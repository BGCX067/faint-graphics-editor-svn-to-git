// -*- coding: us-ascii-unix -*-
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

#include <cassert>
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "commands/resize-cmd.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/point.hh"
#include "objects/object.hh"
#include "text/utf8-string.hh"
#include "util/object-util.hh"
#include "util/optional.hh"

namespace faint{

static void offset_for_resize(CommandContext& context, const IntPoint& offset){
  offset_by(context.GetObjects(), offset);
  context.OffsetRasterSelectionOrigin(offset);
}

class ResizeCommand : public Command {
public:
  ResizeCommand(const IntRect& rect, const Paint& bg, const utf8_string& name)
    : Command(CommandType::HYBRID),
      m_bg(bg),
      m_isDwim(false),
      m_name(name),
      m_rect(rect)
  {}

  ResizeCommand(const IntRect& rect, const AltIntRect& altRect, const Paint& bg, const utf8_string& name)
    : Command(CommandType::HYBRID),
      m_altRect(altRect.Get()),
      m_bg(bg),
      m_isDwim(false),
      m_name(name),
      m_rect(rect)
  {}

  ResizeCommand(const IntRect& rect, const Paint& bg, const AltPaint& altBg, const utf8_string& name)
    : Command(CommandType::HYBRID),
      m_altBg(altBg.Get()),
      m_bg(bg),
      m_isDwim(false),
      m_name(name),
      m_rect(rect)
  {}

  void Do(CommandContext& context) override{
    DoRaster(context);
    offset_for_resize(context, -m_rect.TopLeft());
  }

  void DoRaster(CommandContext& context) override{
    Bitmap bmp(m_rect.GetSize());
    clear(bmp, m_bg);
    blit(context.GetBitmap(), onto(bmp), -m_rect.TopLeft());
    context.SetBitmap(std::move(bmp));
  }

  Command* GetDWIM() override{
    if (m_altBg.IsSet()){
      ResizeCommand* cmd = new ResizeCommand(m_rect, m_altBg.Get(),
        Alternate(m_bg), "Resize Image (DWIM)");
      cmd->SetDWIM();
      return cmd;
    }
    else if (m_altRect.IsSet()){
      ResizeCommand* cmd = new ResizeCommand(m_altRect.Get(),
        Alternate(m_rect), m_bg, "Resize Image (DWIM)");
      cmd->SetDWIM();
      return cmd;
    }
    assert(false);
    return nullptr;
  }

  bool HasDWIM() const override{
    return m_altBg.IsSet() || m_altRect.IsSet();
  }

  utf8_string Name() const override{
    return m_name;
  }

  IntPoint SelectionOffset() const override{
    return -m_rect.TopLeft();
  }


  Point Translate(const Point& p) const override{
    return p - floated(m_rect.TopLeft());
  }

  void Undo(CommandContext& context) override{
    offset_for_resize(context, m_rect.TopLeft());
  }

  Point UndoTranslate(const Point& p) const override{
    return p + floated(m_rect.TopLeft());
  }

private:
  void SetDWIM(){
    m_isDwim = true;
  }

  Optional<Paint> m_altBg;
  Optional<IntRect> m_altRect;
  Paint m_bg;
  bool m_isDwim;
  utf8_string m_name;
  IntRect m_rect;
};

Command* resize_command(const IntRect& rect, const Paint& src,
  const utf8_string& name)
{
  return new ResizeCommand(rect, src, name);
}

Command* resize_command(const IntRect& rect, const AltIntRect& altRect,
  const Paint& src, const utf8_string& name)
{
  return new ResizeCommand(rect, altRect, src, name);
}

Command* resize_command(const IntRect& rect, const Paint& src,
  const AltPaint& altSrc, const utf8_string& name)
{
  return new ResizeCommand(rect, src, altSrc, name);
}

} // namespace
