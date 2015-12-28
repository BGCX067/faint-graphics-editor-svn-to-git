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
#include "commands/frame-cmd.hh"

class AddFrameCommand : public Command {
public:
  AddFrameCommand( const IntSize& size )
    : Command(CommandType::FRAME)
  {
    ImageInfo info(size);
    FrameProps props(info);
    m_image = new faint::Image(props);
  }

  AddFrameCommand( const faint::Image& frame, const index_t& index )
    : Command(CommandType::FRAME),
      m_image(new faint::Image(frame)),
      m_index(index)
  {}

  ~AddFrameCommand(){
    delete m_image;
  }

  void Do( CommandContext& context ) override{
    if ( m_index.IsSet() ){
      context.AddFrame(m_image, m_index.Get());
    }
    else {
      context.AddFrame(m_image);
    }
  }

  std::string Name() const override{
    return "Add Frame";
  }

  void Undo( CommandContext& context ) override{
    context.RemoveFrame(m_image);
  }

private:
  faint::Image* m_image;
  Optional<index_t> m_index;
};

Command* add_frame_command(const IntSize& size){
  return new AddFrameCommand(size);
}

Command* add_frame_command(const faint::Image& image, const index_t& index){
  return new AddFrameCommand(image, index);
}

class RemoveFrame : public Command {
public:
  RemoveFrame(const index_t& index)
    : Command(CommandType::FRAME),
      m_image(nullptr),
      m_index(index)
  {}

  void Do(CommandContext& ctx) override{
    if ( m_image == nullptr ){
      // Retrieve the image address on the first run for undo
      m_image = &ctx.GetFrame(m_index);
    }
    ctx.RemoveFrame(m_index);
  }

  void Undo(CommandContext& ctx) override{
    assert(m_image != nullptr);
    ctx.AddFrame(m_image, m_index);
  }

  std::string Name() const override{
    return "Remove Frame";
  }
private:
  faint::Image* m_image;
  index_t m_index;
};

Command* remove_frame_command( const index_t& index ){
  return new RemoveFrame(index);
}

class ReorderFrame : public Command {
public:
  ReorderFrame(const new_index_t& newIndex, const old_index_t& oldIndex)
    : Command(CommandType::FRAME),
      m_newIndex(newIndex),
      m_oldIndex(oldIndex)
  {}

  void Do(CommandContext& ctx) override{
    ctx.ReorderFrame(m_newIndex, m_oldIndex);
  }

  void Undo(CommandContext& ctx) override{
    ctx.ReorderFrame(New(m_oldIndex.Get()), Old(m_newIndex.Get()));
  }

  std::string Name() const override{
    return "Reorder Frames";
  }
private:
  new_index_t m_newIndex;
  old_index_t m_oldIndex;
};

Command* reorder_frame_command( const new_index_t& newIndex, const old_index_t& oldIndex ){
  return new ReorderFrame(newIndex, oldIndex);
}

class SetFrameDelay : public Command {
public:
  SetFrameDelay(const index_t& frameIndex, const NewDelay& newDelay, const OldDelay& oldDelay)
    : Command(CommandType::FRAME),
      m_frameIndex(frameIndex),
      m_newDelay(newDelay.Get()),
      m_oldDelay(oldDelay.Get())
  {}

  void Do(CommandContext& ctx) override{
    ctx.SetFrameDelay(m_frameIndex, m_newDelay);
  }

  void Undo(CommandContext& ctx) override{
    ctx.SetFrameDelay(m_frameIndex, m_oldDelay);
  }

  std::string Name() const override{
    return "Set Frame Delay";
  }
private:
  index_t m_frameIndex;
  delay_t m_newDelay;
  delay_t m_oldDelay;
};

Command* set_frame_delay_command( const index_t& frameIndex, const NewDelay& newDelay, const OldDelay& oldDelay ){
  return new SetFrameDelay(frameIndex, newDelay, oldDelay);
}

class SetFrameHotSpot : public Command {
public:
  SetFrameHotSpot(const index_t& frameIndex, const NewHotSpot& newHotSpot, const OldHotSpot& oldHotSpot)
    : Command(CommandType::FRAME),
      m_frameIndex(frameIndex),
      m_newHotSpot(newHotSpot.Get()),
      m_oldHotSpot(oldHotSpot.Get())

  {}

  void Do(CommandContext& ctx) override{
    ctx.SetFrameHotSpot(m_frameIndex, m_newHotSpot);
  }

  void Undo(CommandContext& ctx) override{
    ctx.SetFrameHotSpot(m_frameIndex, m_oldHotSpot);
  }

  std::string Name() const override{
    return "Set Frame HotSpot";
  }
private:
  index_t m_frameIndex;
  IntPoint m_newHotSpot;
  IntPoint m_oldHotSpot;
};

Command* set_frame_hotspot_command( const index_t& frameIndex, const NewHotSpot& newHotSpot, const OldHotSpot& oldHotSpot ){
  return new SetFrameHotSpot(frameIndex, newHotSpot, oldHotSpot);
}
