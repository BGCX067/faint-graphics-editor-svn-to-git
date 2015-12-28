#include "objects/objtext.hh"
#include "rendering/overlay.hh"
#include "tasks/edit-text.hh"
#include "tasks/select-text.hh"

SelectText::SelectText( ObjText* obj, bool newTextObject, const Point& clickPos, Settings& s )
  : m_newTextObject(newTextObject),
    m_origin(clickPos),
    m_settings(s),
    m_textObject(obj)
{
  m_textObject->SetCaretPos( m_textObject->CaretPos(clickPos), false );
}

void SelectText::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  overlays.Corners( m_textObject->GetTri() );
  if ( m_newTextObject ){
    m_textObject->Draw(dc);
  }

  if ( !m_textObject->HasSelectedRange() ){
    overlays.Caret( m_textObject->GetCaret() );
  }
}

bool SelectText::DrawBeforeZoom(Layer layer) const{
  return layer == Layer::RASTER;
}

Command* SelectText::GetCommand(){
  return nullptr;
}

Cursor SelectText::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CARET;
}

Task* SelectText::GetNewTask(){
  return nullptr;
}

IntRect SelectText::GetRefreshRect( const IntRect&, const Point& ) const{
  return m_textObject->GetRefreshRect();
}

TaskResult SelectText::LeftDown( const CursorPositionInfo& ){
  // The task is created on left down
  return TASK_NONE;
}

TaskResult SelectText::LeftUp( const CursorPositionInfo& ){
  return TASK_CHANGE;
}

TaskResult SelectText::Motion( const CursorPositionInfo& info ){
  m_textObject->SetCaretPos( m_textObject->CaretPos(info.pos), true );
  return TASK_DRAW;
}

TaskResult SelectText::Preempt( const CursorPositionInfo& ){
  return TASK_NONE;
}
