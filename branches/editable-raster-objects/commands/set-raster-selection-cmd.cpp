#include "commands/set-raster-selection-cmd.hh"

SetRasterSelection::SetRasterSelection(const NewRasterSelectionState& newState, const OldRasterSelectionState& oldState, const std::string& name, bool appendCommand )
  : Command( CommandType::SELECTION ),
    m_name(name),
    m_newState(newState.Get()),
    m_oldState(oldState.Get()),
    m_appendCommand(appendCommand)
{}

void SetRasterSelection::Do( CommandContext& ctx ){
  ctx.SetRasterSelection(m_newState);
}

bool SetRasterSelection::ModifiesState() const{
  // Moving a floating selection or changing to/from floating state
  // affect the image content, merely selecting a region does not.
  return m_newState.Floating() || m_oldState.Floating();
}

std::string SetRasterSelection::Name() const{
  return m_name;
}

void SetRasterSelection::Undo( CommandContext& ctx ){
  ctx.SetRasterSelection(m_oldState);
}

bool SetRasterSelection::ShouldAppend() const{
  return m_appendCommand;
}

class MoveRasterSelectionCommand : public Command {
public:
  MoveRasterSelectionCommand(const IntPoint& newPos, const IntPoint& oldPos )
  : Command(CommandType::SELECTION),
    m_newPos(newPos),
    m_oldPos(oldPos)
  {}

  ~MoveRasterSelectionCommand(){
    for ( Command* cmd : m_merged ){
      delete cmd;
    }
  }

  bool Merge( Command* cmd ) override{
    MoveRasterSelectionCommand* candidate =
      dynamic_cast<MoveRasterSelectionCommand*>(cmd);
    if ( candidate == nullptr ){
      return false;
    }

    m_merged.push_back(candidate);
    m_newPos = candidate->m_newPos;
    return true;
  }

  void Do( CommandContext& ctx ) override{
    ctx.MoveRasterSelection(m_newPos);
  }

  bool ModifiesState() const override{
    return true;
  }

  std::string Name() const override{
    return "Move Selected Content";
  }

  void Undo( CommandContext& ctx ) override{
    ctx.MoveRasterSelection(m_oldPos);
  }
private:
  std::vector<Command*> m_merged;
  IntPoint m_newPos;
  IntPoint m_oldPos;
};

Command* move_raster_selection_command(const IntPoint& newPos, const IntPoint& oldPos ){
  return new MoveRasterSelectionCommand(newPos, oldPos);
}

bool is_move_raster_selection_command(Command* cmd){
  return dynamic_cast<MoveRasterSelectionCommand*>(cmd) != nullptr;
}

SetSelectionOptions::SetSelectionOptions(const NewRasterSelectionOptions& newOptions, const OldRasterSelectionOptions& oldOptions )
  : Command( CommandType::SELECTION ),
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
