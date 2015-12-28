// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#include "commands/set-raster-selection-cmd.hh"
#include "geo/geo-func.hh"
#include "geo/tri.hh"
#include "rendering/faint-dc.hh"
#include "text/utf8-string.hh"
#include "util/optional.hh"
#include "util/setting-util.hh"

namespace faint{

class SetSelectionOptionsCommand : public Command {
public:
  SetSelectionOptionsCommand(const NewSelectionOptions& newOptions, const OldSelectionOptions& oldOptions)
    : Command(CommandType::SELECTION),
      m_newOptions(newOptions.Get()),
      m_oldOptions(oldOptions.Get())
  {}

  void Do(CommandContext& ctx) override{
    ctx.SetRasterSelectionOptions(m_newOptions);
  }

  utf8_string Name() const override{
    return "Change Selection Settings";
  }

  void Undo(CommandContext& ctx) override{
    ctx.SetRasterSelectionOptions(m_oldOptions);
  }
private:
  SelectionOptions m_newOptions;
  SelectionOptions m_oldOptions;
};

class SetRasterSelectionCommand : public Command{
public:
  SetRasterSelectionCommand(const NewSelectionState& newState, const OldSelectionState& oldState, const utf8_string& name, bool appendCommand)
    : Command(CommandType::SELECTION),
      m_appendCommand(appendCommand),
      m_name(name),
      m_newState(newState.Get()),
      m_oldState(oldState.Get()),
      m_optionsCommand(nullptr)
  {}

  ~SetRasterSelectionCommand(){
    delete m_optionsCommand;
  }

  void Do(CommandContext& ctx) override{
    if (m_optionsCommand != nullptr){
      m_optionsCommand->Do(ctx);
    }
    ctx.SetRasterSelection(m_newState);

  }

  bool ModifiesState() const override{
    // Moving a floating selection or changing to/from floating state
    // affect the image content, merely selecting a region does not.
    return m_newState.Floating() || m_oldState.Floating();
  }

  utf8_string Name() const override{
    return m_name;
  }

  void Undo(CommandContext& ctx) override{
    ctx.SetRasterSelection(m_oldState);
    if (m_optionsCommand != nullptr){
      m_optionsCommand->Undo(ctx);
    }
  }

  bool ShouldAppend() const { // Fixme: Remove?
    return m_appendCommand;
  }

  void SetOptionsCommand(SetSelectionOptionsCommand* cmd) { // Fixme: Remove?
    m_optionsCommand = cmd;
  }

  SetRasterSelectionCommand& operator=(const SetRasterSelectionCommand&) = delete;
private:
  bool m_appendCommand;
  const utf8_string m_name;
  SelectionState m_newState;
  SelectionState m_oldState;
  SetSelectionOptionsCommand* m_optionsCommand;
};

class MoveRasterSelectionCommand : public Command {
public:
  MoveRasterSelectionCommand(const IntPoint& newPos, const IntPoint& oldPos)
    : Command(CommandType::SELECTION),
      m_newPos(newPos),
      m_oldPos(oldPos)
  {}

  ~MoveRasterSelectionCommand(){
    for (Command* cmd : m_merged){
      delete cmd;
    }
  }

  bool Merge(Command* cmd, bool sameFrame) override{
    if (!sameFrame){
      return false;
    }
    MoveRasterSelectionCommand* candidate =
      dynamic_cast<MoveRasterSelectionCommand*>(cmd);
    if (candidate == nullptr){
      return false;
    }

    m_merged.push_back(candidate);
    m_newPos = candidate->m_newPos;
    return true;
  }

  void Do(CommandContext& ctx) override{
    ctx.MoveRasterSelection(m_newPos);
  }

  bool ModifiesState() const override{
    return true;
  }

  utf8_string Name() const override{
    return "Move Selected Content";
  }

  void Undo(CommandContext& ctx) override{
    ctx.MoveRasterSelection(m_oldPos);
  }
private:
  std::vector<Command*> m_merged;
  IntPoint m_newPos;
  IntPoint m_oldPos;
};

Command* move_raster_selection_command(const IntPoint& newPos, const IntPoint& oldPos){
  return new MoveRasterSelectionCommand(newPos, oldPos);
}

bool is_move_raster_selection_command(Command* cmd){
  return dynamic_cast<MoveRasterSelectionCommand*>(cmd) != nullptr;
}

class StampFloatingSelectionCommand : public Command {
public:
  StampFloatingSelectionCommand(const Bitmap& bmp, const IntRect& r, const Optional<IntRect>& oldRect, const SelectionOptions& options)
    : Command(CommandType::RASTER),
      m_bitmap(copy(bmp)),
      m_oldRect(oldRect),
      m_rect(r),
      m_name("Stamp selection")
  {
    m_settings = bitmap_mask_settings(options.mask, options.bg, options.alpha);
  }

  void Do(CommandContext& context) override{
    FaintDC& dc(context.GetDC());
    if (m_oldRect.IsSet()){
      dc.Rectangle(tri_from_rect(floated(m_oldRect.Get())),
        eraser_rectangle_settings(m_settings.Get(ts_Bg)));
    }
    dc.Blit(m_bitmap, floated(m_rect.TopLeft()), m_settings);
  }

  utf8_string Name() const override{
    return m_name;
  }
  StampFloatingSelectionCommand(const StampFloatingSelectionCommand&) = delete;
private:
  Bitmap m_bitmap;
  Optional<IntRect> m_oldRect;
  IntRect m_rect;
  Settings m_settings;
  utf8_string m_name;
};

Command* stamp_floating_selection_command(const RasterSelection& selection){
  assert(selection.Floating());
  Optional<IntRect> oldRect;
  if (!selection.Copying()){
    oldRect.Set(selection.GetOldRect());
  }
  return new StampFloatingSelectionCommand(selection.GetBitmap(), selection.GetRect(), oldRect, selection.GetOptions());
}

Command* stamp_floating_selection_command(const sel::Copying& copying){
  return new StampFloatingSelectionCommand(copying.GetBitmap(), copying.Rect(), no_option(), copying.GetOptions());
}

Command* stamp_floating_selection_command(const sel::Moving& moving){
  return new StampFloatingSelectionCommand(moving.GetBitmap(), moving.Rect(), option(moving.OldRect()), moving.GetOptions());
}

Command* set_selection_options_command(const NewSelectionOptions& newOptions, const OldSelectionOptions& oldOptions){
  return new SetSelectionOptionsCommand(newOptions, oldOptions);
}

Command* set_raster_selection_command(const NewSelectionState& newState,const OldSelectionState& oldState, const utf8_string& name, bool appendCommand){
  return new SetRasterSelectionCommand(newState, oldState, name, appendCommand);
}

Command* set_raster_selection_command(const NewSelectionState& newState,const OldSelectionState& oldState, const utf8_string& name, bool appendCommand, const NewSelectionOptions& newOptions, const OldSelectionOptions& oldOptions){

  SetRasterSelectionCommand* cmd = new SetRasterSelectionCommand(newState, oldState, name, appendCommand);
  cmd->SetOptionsCommand(new SetSelectionOptionsCommand(newOptions, oldOptions));
  return cmd;
}

bool is_appendable_raster_selection_command(Command* cmd){
  SetRasterSelectionCommand* candidate =
    dynamic_cast<SetRasterSelectionCommand*>(cmd);
  if (candidate == nullptr || !candidate->ShouldAppend()){
    return false;
  }
  return true;
}

} // namespace
