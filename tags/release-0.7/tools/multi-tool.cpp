#include "multi-tool.hh"

MultiTool::MultiTool( ToolId id, default_task defaultTask, initial_task initialTask )
  : Tool(id),
    m_task(defaultTask, initialTask)
{}

MultiTool::MultiTool( ToolId id, SettingNotifier& notifier, default_task defaultTask, initial_task initialTask )
  : Tool(id, notifier),
    m_task(defaultTask, initialTask)
{}

MultiTool::MultiTool( ToolId id, SettingNotifier& notifier, ToolSettingMode settingMode, default_task defaultTask, initial_task initialTask)
  : Tool(id, notifier, settingMode),
    m_task(defaultTask, initialTask)
{}

bool MultiTool::AcceptsPastedText() const{
  return m_task->AcceptsPastedText();
}

bool MultiTool::CanUndo() const{
  return m_task->CanUndo();
}

bool MultiTool::CanRedo() const{
  return m_task->CanRedo();
}

ToolResult MultiTool::Char( const KeyInfo& info ){
  return HandleTaskResult(m_task->Char(info));
}

bool MultiTool::CopyText( faint::utf8_string& str, bool clear){
  return m_task->CopyText(str, clear);
}

ToolResult MultiTool::Delete(){
  return HandleTaskResult(m_task->Delete());
}

ToolResult MultiTool::Deselect(){
  return HandleTaskResult(m_task->Deselect());
}

bool MultiTool::Draw( FaintDC& dc, Overlays& overlays, const Point& pt ){
  return m_task->Draw(dc, overlays, pt);
}

bool MultiTool::DrawBeforeZoom(Layer::type layer) const{
  return m_task->DrawBeforeZoom(layer);
}

Command* MultiTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type MultiTool::GetCursor(const CursorPositionInfo& info) const{
  return m_task->GetCursor(info);
}

std::string MultiTool::GetRedoName() const{
  return m_task->GetRedoName();
}

IntRect MultiTool::GetRefreshRect(const IntRect& visible, const Point& currPos) const{
  return m_task->GetRefreshRect(visible, currPos);
}

std::string MultiTool::GetUndoName() const{
  return m_task->GetUndoName();
}

bool MultiTool::HasSelection() const{
  return m_task->HasSelection();
}

ToolResult MultiTool::LeftDoubleClick( const CursorPositionInfo& info ){
  return HandleTaskResult(m_task->LeftDoubleClick(info));
}

ToolResult MultiTool::LeftDown( const CursorPositionInfo& info ){
  return HandleTaskResult(m_task->LeftDown(info));
}

ToolResult MultiTool::LeftUp( const CursorPositionInfo& info ){
  return HandleTaskResult(m_task->LeftUp(info));
}

ToolResult MultiTool::Motion( const CursorPositionInfo& info ){
  return HandleTaskResult(m_task->Motion(info));
}

void MultiTool::Paste( const faint::utf8_string& str ){
  m_task->Paste(str);
}

ToolResult MultiTool::Preempt( const CursorPositionInfo& info ){
  return HandleTaskResult(m_task->Preempt( info ));
}

void MultiTool::Redo(){
  m_task->Redo();
}

void MultiTool::ResetTask( default_task defaultTask, initial_task initialTask ){
  m_task.Reset( defaultTask, initialTask );
}

ToolResult MultiTool::SelectAll(){
  return HandleTaskResult(m_task->SelectAll());
}

void MultiTool::SelectionChange(){
  m_task->SelectionChange();
}

void MultiTool::Undo(){
  m_task->Undo();
}

void MultiTool::UpdateTaskSettings(){
  m_task->UpdateSettings();
}

ToolResult MultiTool::HandleTaskResult(TaskResult r){
  if ( r == TASK_DRAW ){
    return TOOL_DRAW;
  }
  else if ( r == TASK_CHANGE ){
    if ( m_task.Switch() ){
      return TOOL_DRAW;
    }
    return TOOL_CHANGE;
  }
  else if ( r == TASK_COMMIT ){
    m_command.Set( m_task->GetCommand() );
    return TOOL_COMMIT;
  }
  else if ( r == TASK_COMMIT_AND_CHANGE ){
    m_command.Set( m_task->GetCommand() );
    if ( m_task.Switch() ){
      return TOOL_COMMIT;
    }
    return TOOL_CHANGE;
  }
  else if ( r == TASK_PUSH ){
    m_task.Push();
    return TOOL_DRAW;
  }
  return TOOL_NONE;
}
