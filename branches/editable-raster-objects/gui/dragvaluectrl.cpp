#include <sstream>
#include "app/getappcontext.hh"
#include "gui/cursors.hh"
#include "gui/dragvaluectrl.hh"
#include "util/angle.hh"
#include "util/convertwx.hh"
#include "util/util.hh"

DEFINE_EVENT_TYPE( EVT_DRAG_VALUE_CHANGE )

const wxColour g_highlightColor(wxColour(0,0,0));
const wxColour g_originalColor(wxColour(40,40,40));

int drag_value( const Point& p0, const Point& p1, int value_p0, const IntRange& range ){
  int d = truncated(distance(p0, p1));
  faint::degree a = angle360(p0.x, p0.y, p1.x, p1.y);
  if ( 135 < a && a < 315 ){
    d *= -1;
  }
  return range.Constrain(value_p0 -d);
}

DragValueCtrl::DragValueCtrl(wxWindow* parent, const IntRange& range, const description_t& statusText )
  : wxPanel(parent, wxID_ANY),
    m_currentValue(10),
    m_originValue(10),
    m_range(range),
    m_statusText(statusText.Get())
{
  wxSize sz(GetSize());
  SetForegroundColour(g_originalColor);
  SetCursor(to_wx_cursor(Cursor::MOVE_POINT));
  SetInitialSize(wxSize(50,40));
}

void DragValueCtrl::OnCaptureLost(wxMouseCaptureLostEvent&){
  SetForegroundColour(g_originalColor);
  m_originValue = m_currentValue = drag_value(m_current, m_origin, m_originValue, m_range);
  SendChangeEvent(m_originValue);
}

void DragValueCtrl::OnLeftDown(wxMouseEvent& evt){
  wxPoint pos(evt.GetPosition());
  m_origin.x = pos.x;
  m_origin.y = pos.y;
  m_current = m_origin;
  SetForegroundColour(g_highlightColor);
  CaptureMouse();
  GetAppContext().GetStatusInfo().SetMainText("");
}
void DragValueCtrl::OnLeftUp(wxMouseEvent& evt){
  SetForegroundColour(g_originalColor);
  Refresh();
  if ( HasCapture() ){
    ReleaseMouse();
  }
  m_originValue = m_currentValue = drag_value( m_current, m_origin, m_originValue, m_range );

  wxPoint pos = evt.GetPosition();
  wxSize size = GetSize();
  if ( pos.x < size.GetWidth() && pos.y < size.GetHeight() && pos.x > 0 && pos.y > 0 ){
    GetAppContext().GetStatusInfo().SetMainText(m_statusText);
  }
  Refresh();
  SendChangeEvent(m_originValue);

}

void DragValueCtrl::OnMotion(wxMouseEvent& evt ){
  if ( HasCapture() ){
    wxPoint pos(evt.GetPosition());
    m_current.x = pos.x;
    m_current.y = pos.y;

    int newValue = drag_value( m_current, m_origin, m_originValue, m_range );
    if ( newValue != m_currentValue ){
      m_currentValue = newValue;
      Refresh();
      Update(); // Required to avoid long delay in refreshing the number
      SendChangeEvent(m_currentValue);
    }
  }
}

void DragValueCtrl::OnMouseEnter(wxMouseEvent&){
  if ( !HasCapture() ){
    GetAppContext().GetStatusInfo().SetMainText(m_statusText);
  }
}

void DragValueCtrl::OnMouseLeave(wxMouseEvent&){
  GetAppContext().GetStatusInfo().SetMainText("");
}

void DragValueCtrl::OnPaint(wxPaintEvent& ){
  wxPaintDC dc(this);
  std::stringstream ss;
  ss << m_currentValue;
  wxString str(ss.str());
  wxCoord textWidth, textHeight;
  dc.GetTextExtent( str, &textWidth, &textHeight );
  wxSize winSize(GetSize());
  dc.DrawText(ss.str(), winSize.GetWidth() / 2 - textWidth / 2 , winSize.GetHeight() / 2 - textHeight / 2);
}

void DragValueCtrl::SendChangeEvent( int value ){
  wxCommandEvent event(EVT_DRAG_VALUE_CHANGE, GetId());
  event.SetEventObject(this);
  event.SetInt( value );
  ProcessEvent(event);
}

void DragValueCtrl::SetValue( int value ){
  m_originValue = m_currentValue = value;
  Refresh();
}

BEGIN_EVENT_TABLE(DragValueCtrl, wxPanel)
EVT_LEFT_DOWN(DragValueCtrl::OnLeftDown)
EVT_LEFT_UP(DragValueCtrl::OnLeftUp)
EVT_MOTION(DragValueCtrl::OnMotion)
EVT_MOUSE_CAPTURE_LOST(DragValueCtrl::OnCaptureLost)
EVT_ENTER_WINDOW(DragValueCtrl::OnMouseEnter)
EVT_LEAVE_WINDOW(DragValueCtrl::OnMouseLeave)
EVT_PAINT(DragValueCtrl::OnPaint)
END_EVENT_TABLE()
