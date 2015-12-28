#include "statusbutton.hh"

StatusButton::StatusButton( wxWindow* parent, const wxSize& size, StatusInterface& status, const std::string& label, const tooltip_t& tooltip, const description_t& description )
  : wxButton(parent, wxNewId(), label, wxDefaultPosition, size, wxWANTS_CHARS ), // wants chars avoids sound when key has focus
    m_description(description.Get()),
    m_status(status)
{
  SetToolTip(tooltip.Get());
}

void StatusButton::OnLeave( wxMouseEvent& event ){
  m_status.SetMainText("");
  event.Skip();
}

void StatusButton::OnMotion( wxMouseEvent& event ){
  // Set the status bar description. Done in EVT_MOTION-handler
  // because it did not work well in EVT_ENTER (the enter event
  // appears to be missed for example when moving between buttons)
  m_status.SetMainText(m_description);
  event.Skip();
}

void StatusButton::UpdateText( const std::string& label, const tooltip_t& tooltip, const description_t& description ){
  SetLabel(label);
  m_description = description.Get();
  SetToolTip(tooltip.Get());
}

BEGIN_EVENT_TABLE(StatusButton, wxButton)
EVT_LEAVE_WINDOW( StatusButton::OnLeave )
EVT_MOTION( StatusButton::OnMotion )
END_EVENT_TABLE()

ToggleStatusButton::ToggleStatusButton( wxWindow* parent, int id, const wxSize& size, StatusInterface& status, const wxBitmap& bmp, const tooltip_t& tooltip, const description_t& description )
: wxBitmapToggleButton(parent, id, bmp, wxDefaultPosition, size, wxWANTS_CHARS),
  m_description(description.Get()),
  m_status(status)
{
  SetToolTip(tooltip.Get());
}

void ToggleStatusButton::OnLeave( wxMouseEvent& event ){
  m_status.SetMainText("");
  event.Skip();
}

void ToggleStatusButton::OnMotion( wxMouseEvent& event ){
  // Set the status bar description. Done in EVT_MOTION-handler
  // because it did not work well in EVT_ENTER (the enter event
  // appears to be missed for example when moving between buttons)
  m_status.SetMainText(m_description);
  event.Skip();
}

BEGIN_EVENT_TABLE(ToggleStatusButton, wxBitmapToggleButton)
EVT_LEAVE_WINDOW( ToggleStatusButton::OnLeave )
EVT_MOTION( ToggleStatusButton::OnMotion )
END_EVENT_TABLE()

