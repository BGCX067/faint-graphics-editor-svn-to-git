#include "commands/set-raster-selection-cmd.hh"

SetRasterSelection::SetRasterSelection(const NewRasterSelectionState& newState, const OldRasterSelectionState& oldState, const std::string& name )
  : Command( CMD_TYPE_OBJECT ), // Fixme: Perhaps add CMD_TYPE_SELECTION or something
    m_newState(newState.Get()),
    m_oldState(oldState.Get()),
    m_name(name)
{}

void SetRasterSelection::Do( CommandContext& ctx ){
  ctx.SetRasterSelection(m_newState);
}

std::string SetRasterSelection::Name() const{
  return m_name;
}

void SetRasterSelection::Undo( CommandContext& ctx ){
  ctx.SetRasterSelection(m_oldState);
}

SetSelectionOptions::SetSelectionOptions(const NewRasterSelectionOptions& newOptions, const OldRasterSelectionOptions& oldOptions )
  : Command( CMD_TYPE_OBJECT ), // Fixme: Perhaps add CMD_TYPE_SELECTION
    m_newOptions(newOptions.Get()),
    m_oldOptions(oldOptions.Get())
{}

void SetSelectionOptions::Do( CommandContext& ctx ){
  ctx.SetRasterSelectionOptions(m_newOptions);
}

std::string SetSelectionOptions::Name() const{
  return "Change Selection Settings";
}

void SetSelectionOptions::Undo( CommandContext& ctx ){
  ctx.SetRasterSelectionOptions(m_oldOptions);
}
