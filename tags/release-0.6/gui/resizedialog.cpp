#include "wx/textctrl.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/checkbox.h"
#include "wx/button.h"
#include "wx/accel.h"
#include "wx/dcmemory.h"
#include "app/app.hh" // Fixme: For retrieving ArtContainer
#include "gui/resizedialog.hh"
#include "gui/valuetextctrl.hh"
#include "util/artcontainer.hh"
#include "util/colorutil.hh"

int ID_TOP_LEFT = wxNewId();
int ID_CENTER = wxNewId();
int ID_SCALE = wxNewId();
int ID_TOGGLE_CHECK = wxNewId();

const wxSize buttonSize(85, 100);

ButtonWithLabel::ButtonWithLabel(){
  label = 0;
  button = 0;
}

bool ButtonWithLabel::Exists() const{
  return this->button != 0;
}

ValueTextControl* AddEntryField( wxWindow* parent, wxBoxSizer* sizer, const wxString& labelStr, int value ){
  wxStaticText* label = new wxStaticText( parent, wxID_ANY, labelStr );
  sizer->Add( label, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 10 );
  ValueTextControl* ctrl = new ValueTextControl( parent, value);
  ctrl->FitSizeTo("1024 (100%)");
  sizer->Add( ctrl, 0, wxALIGN_CENTER_VERTICAL );
  return ctrl;
}

ButtonWithLabel AddButton( wxWindow* parent, wxBoxSizer* sizer, int id, wxString label, wxString toolTip, const wxBitmap& bmp ){
  ButtonWithLabel buttonAndLabel;
  buttonAndLabel.button = new wxBitmapButton( parent, id, bmp,
     wxDefaultPosition, wxDefaultSize);
  buttonAndLabel.button->SetToolTip(toolTip);
  buttonAndLabel.label = new wxStaticText( parent, wxID_ANY, "(Default)" );
  buttonAndLabel.button->SetInitialSize( buttonSize );

  wxBoxSizer* oneButtonSizer = new wxBoxSizer( wxVERTICAL );
  oneButtonSizer->Add( buttonAndLabel.button );
  oneButtonSizer->Add( buttonAndLabel.label, 0, wxALIGN_CENTER );
  sizer->Add( oneButtonSizer );
  return buttonAndLabel;
}

const IntSize g_resize_bitmap_size(58,62);

wxBitmap CreateResizeBitmap(const faint::Color& c, const IntSize& size, const wxBitmap& image, const wxPoint& imagePos){
  wxBitmap bmp( color_bitmap(c, size, false) );
  {
    wxMemoryDC dc(bmp);
    dc.DrawBitmap(image,imagePos);
  }
  return bmp;
}

ResizeDialog::ResizeDialog( wxWindow* parent, const wxString& title, const ResizeDialogSettings& settings )
  : wxDialog( parent, wxID_ANY, title)
{
  const ArtContainer& icons = wxGetApp().GetArtContainer();
  // The outermost vertical sizer
  wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

  // A proportion checkbox
  wxBoxSizer* row1 = new wxBoxSizer( wxHORIZONTAL );
  m_proportional = new wxCheckBox( this, wxID_ANY, "&Proportional" );
  m_proportional->SetValue( settings.proportional );
  row1->Add( m_proportional, 0 );
  vSizer->Add( row1, 0, wxALL, 10 );

  // The row with the width and height entry fields
  wxBoxSizer* row2 = new wxBoxSizer( wxHORIZONTAL );
  m_widthCtrl = AddEntryField( this, row2, "&Width", settings.w );
  row2->AddSpacer(20);
  m_lastChanged = m_widthCtrl;
  m_heightCtrl = AddEntryField( this, row2, "&Height", settings.h );
  vSizer->Add(row2, 0, wxALL, 10 );
  // The buttons for resize type choice
  wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  m_placementBmp = icons.Get("resizedlg_placement");
  if ( !settings.scaleOnly ){
    m_anchorTopLeft = AddButton( this, buttonSizer, ID_TOP_LEFT, "Resize (top left)",
      "Resize drawing area right and down (Key: Q)",
      color_bitmap(faint::Color(255,255,255), g_resize_bitmap_size, false));
    m_anchorCenter = AddButton( this, buttonSizer, ID_CENTER, "Resize (center)",
      "Resize drawing area in all directions (Key: W)",
      color_bitmap(faint::Color(255,255,255), g_resize_bitmap_size, false));
  }
  m_scale = AddButton( this, buttonSizer, ID_SCALE, "Scale",
    "Scale the image (Key: E)" ,
    icons.Get( "resizedlg_scale" ) );
  m_cancel = new wxButton(this, wxID_CANCEL, wxEmptyString,
    wxDefaultPosition, buttonSize );
  buttonSizer->Add( m_cancel, 0 );
  vSizer->Add(buttonSizer, 0, wxALL, 10 );
  this->SetSizerAndFit(vSizer);
  m_widthCtrl->SetFocus();
  m_resizeType = ResizeDialogSettings::UNKNOWN;
  if ( settings.scaleOnly ){
    SetDefaultResizeOption( ResizeDialogSettings::RESCALE );
    // There's no reason to indicate the default when only one option
    // is available
    m_scale.label->Hide();
  }
  else {
    SetDefaultResizeOption( settings.defaultButton );
  }

  wxAcceleratorEntry entries[5];
  // Use Alt+P as an accelerator, so that the check-box
  // doesn't steal the focus
  entries[0].Set( wxACCEL_ALT, (int)'P', ID_TOGGLE_CHECK );
  entries[1].Set( wxACCEL_NORMAL, (int)'P', ID_TOGGLE_CHECK );
  entries[2].Set( wxACCEL_NORMAL, (int)'Q', ID_TOP_LEFT );
  entries[3].Set( wxACCEL_NORMAL, (int)'W', ID_CENTER );
  entries[4].Set( wxACCEL_NORMAL, (int)'E', ID_SCALE );
  SetAcceleratorTable( wxAcceleratorTable(5, entries) );

  // Center on parent
  Centre(wxBOTH);
}

void ResizeDialog::SetBgColor( const faint::Color& c ){
  if ( m_anchorTopLeft.Exists() ){
    // Update the background color of the resize-buttons
    m_anchorTopLeft.button->SetBitmap( CreateResizeBitmap( c, g_resize_bitmap_size, m_placementBmp, wxPoint(0,0) ) );
    m_anchorCenter.button->SetBitmap( CreateResizeBitmap( c, g_resize_bitmap_size, m_placementBmp, wxPoint(16,16) ) );
  }
}

ResizeDialogSettings ResizeDialog::GetSelection() const{
  return ResizeDialogSettings( m_widthCtrl->GetValue(), m_heightCtrl->GetValue(), m_proportional->GetValue(), false, m_resizeType );
}

void ResizeDialog::SetDefaultResizeOption( ResizeDialogSettings::ResizeType type ){
  if ( m_scale.button != 0 ){
    m_scale.label->Hide();
  }
  if ( m_anchorCenter.button != 0 ){
    m_anchorCenter.label->Hide();
  }
  if ( m_anchorTopLeft.button != 0 ){
    m_anchorTopLeft.label->Hide();
  }

  if ( type == ResizeDialogSettings::RESCALE ){
    SetDefaultItem( m_scale.button );
    m_scale.label->Show();
  }
  else if ( type == ResizeDialogSettings::RESIZE_CENTER ){
    SetDefaultItem( m_anchorCenter.button );
    m_anchorCenter.label->Show();
  }
  else {
    SetDefaultItem( m_anchorTopLeft.button );
    m_anchorTopLeft.label->Show();
  }
}

void ResizeDialog::OnValueUpdate( wxCommandEvent& event ){
  m_lastChanged = event.GetEventObject();
  UpdateProportions( true );
}

void ResizeDialog::OnToggleProportional( wxCommandEvent& ){
  UpdateProportions( false );
}

void ResizeDialog::OnTopLeftResize( wxCommandEvent& ){
  m_anchorTopLeft.button->SetFocus();
  if ( GoodValues() && ChangedValues() ){
    m_resizeType = ResizeDialogSettings::RESIZE_TOP_LEFT;
    EndModal(wxID_OK);
  }
  else {
    EndModal(wxID_CANCEL);
  }
}
void ResizeDialog::OnCenterResize( wxCommandEvent& ){
  m_anchorCenter.button->SetFocus();
  if( GoodValues() && ChangedValues() ){
    m_resizeType = ResizeDialogSettings::RESIZE_CENTER;
    EndModal(wxID_OK);
  }
  else {
    EndModal(wxID_CANCEL);
  }
}

void ResizeDialog::OnScale( wxCommandEvent& ){
  m_scale.button->SetFocus();
  if ( GoodValues() && ChangedValues() ){
    m_resizeType = ResizeDialogSettings::RESCALE;
    EndModal(wxID_OK);
  }
  else {
    EndModal(wxID_CANCEL);
    return;
  }
}

bool ResizeDialog::GoodValues() const{
  return m_widthCtrl->GetValue() > 0 &&
    m_heightCtrl->GetValue() > 0;
}

bool ResizeDialog::ChangedValues() const{
  return m_widthCtrl->GetValue() != m_widthCtrl->GetOldValue() ||
    m_heightCtrl->GetValue() != m_heightCtrl->GetOldValue();
}

void ResizeDialog::OnToggleProportionalKB( wxCommandEvent& ){
  m_proportional->SetValue( !m_proportional->GetValue() );
  UpdateProportions(false);
}

void ResizeDialog::UpdateProportions( bool ignoreFocus ){
  // Todo: This could be improved further. Currently, the focused
  // control is never modified like this.
  // - The currently edited field should be updatable if it has not changed
  // after getting focus.
  // Example: edit w, tab to h. Toggle proportional - h should change, not w.
  // However, the edit position etc. should be set properly in h after adjusting, and
  // it should show the editable values (not with percentage etc)
  if ( m_proportional->GetValue() ){
    if ( (!ignoreFocus && m_widthCtrl->HasFocus() ) ||
      ( ( ignoreFocus || !m_heightCtrl->HasFocus() ) && m_lastChanged == m_widthCtrl ) ){
      float ratio = m_widthCtrl->GetValue() / float(m_widthCtrl->GetOldValue());
      m_heightCtrl->SetValue(m_heightCtrl->GetOldValue() * ratio);
    }

    else {
      float ratio = m_heightCtrl->GetValue() / float(m_heightCtrl->GetOldValue());
      m_widthCtrl->SetValue(m_widthCtrl->GetOldValue() * ratio );    }
  }
}

BEGIN_EVENT_TABLE(ResizeDialog, wxDialog)
EVT_COMMAND(-1, EVT_VALUE_TEXT_CONTROL_UPDATE, ResizeDialog::OnValueUpdate)
EVT_CHECKBOX(-1, ResizeDialog::OnToggleProportional)
EVT_MENU(ID_TOGGLE_CHECK, ResizeDialog::OnToggleProportionalKB )
EVT_BUTTON(ID_TOP_LEFT, ResizeDialog::OnTopLeftResize )
EVT_BUTTON(ID_CENTER, ResizeDialog::OnCenterResize )
EVT_BUTTON(ID_SCALE, ResizeDialog::OnScale )
END_EVENT_TABLE()
