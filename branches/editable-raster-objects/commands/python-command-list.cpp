#include "commands/command.hh"
#include "commands/python-command-list.hh"
#include "util/formatting.hh"
#include "util/iter.hh"

class ClosePythonCommandList : public Command {
public:
  ClosePythonCommandList() :
    Command(CommandType::OBJECT)
  {}
  void Do(CommandContext&) override{}
  void Undo(CommandContext&) override{}
  std::string Name() const override{
    return "";
  }
};

bool is_list_closer(Command* cmd){
  return dynamic_cast<ClosePythonCommandList*>(cmd) != nullptr;
}

class OpenPythonCommandList : public Command {
public:
  OpenPythonCommandList()
    : Command(CommandType::HYBRID), // Fixme: Determine dynamically
      m_open(true)
  {}

  ~OpenPythonCommandList(){
    for ( Command* cmd : m_commands ){
      delete cmd;
    }
  }

  void Do( CommandContext& context ) override{
    for ( Command* cmd : m_commands ){
      cmd->Do(context);
    }
  }

  void DoRaster( CommandContext& context ) override{
    for ( Command* cmd : m_commands ){
      cmd->DoRaster( context );
    }
  }

  bool Merge( Command* cmd ) override{
    if ( !m_open ){
      return false;
    }
    m_commands.push_back(cmd);
    if ( is_list_closer(cmd) ){
      m_open = false;
    }
    return true;
  }

  std::string Name() const override{
    return space_sep("Python Commands", bracketed(str_int(NumCommands())));
  }

  void Undo( CommandContext& context ) override{
    for ( Command* cmd : reversed(m_commands) ){
      cmd->Undo( context );
    }
  }

  Command* Unwrap(){
    if ( m_open ){
      return nullptr;
    }

    if ( NumCommands() != 1 ){
      return nullptr;
    }

    Command* unwrapped = m_commands.at(0);
    Command* close = m_commands.back();
    m_commands.pop_back();
    m_commands.pop_back();
    m_commands.push_back(close);
    return unwrapped;
  }

private:
  int NumCommands() const{
    // The number of commands in the list, excluding the close-command
    int count = static_cast<int>(m_commands.size());
    return m_open ? count :
      count - 1;
  }

  commands_t m_commands;
  bool m_open;
};

Command* open_python_command_list(){
  return new OpenPythonCommandList();
}

Command* close_python_command_list(){
  return new ClosePythonCommandList();
}

Command* unwrap_list(Command* cmd){
  OpenPythonCommandList* cmdList = dynamic_cast<OpenPythonCommandList*>(cmd);
  if ( cmdList == nullptr ){
    return nullptr;
  }
  return cmdList->Unwrap();
}
