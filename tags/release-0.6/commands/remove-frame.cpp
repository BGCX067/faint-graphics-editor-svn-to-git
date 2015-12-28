#include "remove-frame.hh"

#include "commands/command.hh"
#include "commands/reorder-frame.hh"

class RemoveFrame : public Command {
public:
  RemoveFrame(size_t oldIndex)
    : Command(CMD_TYPE_OBJECT),
      m_image(0),
      m_oldIndex(oldIndex)
  {}

  void Do(CommandContext& ctx){
    if ( m_image == 0 ){
      m_image = &ctx.GetFrame(m_oldIndex);
    }
    ctx.RemoveFrame(m_oldIndex);
  }

  void Undo(CommandContext& ctx){
    assert(m_image != 0);
    ctx.AddFrame(m_image, m_oldIndex);
  }

  std::string Name() const{
    return "Remove Frame";
  }
private:
  faint::Image* m_image;
  size_t m_oldIndex;
};

Command* remove_frame_command( size_t index ){
  return new RemoveFrame(index);
}

