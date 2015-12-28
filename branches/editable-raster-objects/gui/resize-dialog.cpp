#include <sstream>
#include "wx/bmpbuttn.h"
#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/dialog.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "app/app.hh" // For retrieving ArtContainer
#include "app/getappcontext.hh"
#include "gui/layout.hh"
#include "gui/resize-dialog-settings.hh"
#include "gui/resize-dialog.hh"
#include "gui/valuetextctrl.hh"
#include "util/artcontainer.hh"
#include "util/clipboard.hh"
#include "util/color.hh"
#include "util/colorutil.hh"
#include "util/commandutil.hh"
#include "util/context-commands.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/guiutil.hh"
#include "util/objutil.hh"

class ValueTextControl;
class wxCheckBox;
class ArtContainer;
class wxStaticText;

class ButtonWithLabel{
public:
  ButtonWithLabel()
    : label(nullptr),
      button(nullptr)
  {}

  bool Exists() const{
    return this->button != nullptr;
  }

  void HideLabel(){
    if ( this->label != nullptr ){
      this->label->Hide();
    }
  }

  void SetFocus(){
    if ( this->button != nullptr ){
      this->button->SetFocus();
    }
  }

  void ShowLabel(){
    if ( this->label != nullptr ){
      this->label->Show();
    }
  }
  wxStaticText* label;
  wxBitmapButton* button;
};

static IntRect centered_resize_rect( const IntSize& newSize, const IntSize& oldSize ){
  return IntRect( IntPoint(-(newSize.w - oldSize.w) / 2, -(newSize.h - oldSize.h ) / 2),
    newSize );
}

static wxString get_resize_dialog_title(CanvasInterface& canvas, ApplyTarget target){
  if ( target == APPLY_RASTER_SELECTION ){
    return "Scale Selection";
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    return space_sep("Resize",
      get_collective_name(canvas.GetObjectSelection()));
  }
  assert(target == APPLY_IMAGE);
  return "Resize or Scale Image";
}

static bool get_scale_only(ApplyTarget target){
  return target != APPLY_IMAGE;
}

// Helper for proportional update
static faint::coord get_change_ratio( const ValueTextControl& ctrl ){
  return floated(ctrl.GetValue()) / floated(ctrl.GetOldValue());
}

// Helper for proportional update
static void set_from_ratio( ValueTextControl& ctrl, faint::coord ratio ){
  ctrl.SetValue(rounded(floated(ctrl.GetOldValue()) * ratio ) );
}

static faint::utf8_string serialize_size( const IntSize& size ){
  std::stringstream ss;
  ss << size.w << "," << size.h;
  return to_faint(wxString(ss.str()));
}

static Optional<IntSize> deserialize_size( const faint::utf8_string& str ){
  wxString wxStr(to_wx(str));
  wxArrayString strs = wxSplit(to_wx(str), ',', '\0');
  if ( strs.GetCount() != 2 ){
    return no_option();
  }

  long width;
  if ( !strs[0].ToLong(&width) ){
    return no_option();
  }
  if ( width <= 0 ){
    return no_option();
  }

  long height;
  if ( !strs[1].ToLong(&height) ){
    return no_option();
  }
  if ( height <= 0 ){
    return no_option();
  }

  return option(IntSize(width, height));
}

static wxBitmap create_resize_bitmap(const faint::DrawSource& src, const IntSize& size, const wxBitmap& image, const wxPoint& imagePos){
  IntSize patternSize( size / 2 );
  // Note: clean_bitmap required for using wxMemoryDC::DrawBitmap on
  // MSW. Presumably due to wxWidgets issue #14403.
  wxBitmap bmp(clean_bitmap(to_wx_bmp(with_border(draw_source_bitmap(src, size, patternSize)))));
  {
    wxMemoryDC dc(bmp);
    dc.DrawBitmap(image,imagePos);
  }
  return bmp;
}

static IntSize get_resize_icon_size(){
  return to_faint(wxGetApp().GetArtContainer().Get(Icon::RESIZEDLG_SCALE).GetSize());
}

static ValueTextControl* add_entry_field( wxWindow* parent, wxBoxSizer* sizer, const wxString& labelStr, int value ){
  wxStaticText* label = new wxStaticText( parent, wxID_ANY, labelStr );
  sizer->Add( label, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 10 );
  ValueTextControl* ctrl = new ValueTextControl( parent, value);
  ctrl->FitSizeTo("1024 (100%)");
  sizer->Add( ctrl, 0, wxALIGN_CENTER_VERTICAL );
  return ctrl;
}

static ButtonWithLabel add_button( wxWindow* parent, wxBoxSizer* sizer, int id, wxString toolTip, const wxBitmap& bmp ){
  // Adds a button with the text "(Default)" as a static text label
  // below the button, for indicating which button the Enter-key
  // targets. The label should only be shown for the current default
  // button.

  ButtonWithLabel buttonAndLabel;
  buttonAndLabel.button = new wxBitmapButton( parent, id, bmp,
     wxDefaultPosition, wxDefaultSize);
  buttonAndLabel.button->SetToolTip(toolTip);
  buttonAndLabel.label = new wxStaticText( parent, wxID_ANY, "(Default)" );
  buttonAndLabel.button->SetInitialSize( to_wx(faint::big_button_size) );

  wxBoxSizer* oneButtonSizer = new wxBoxSizer( wxVERTICAL );
  oneButtonSizer->Add( buttonAndLabel.button );
  oneButtonSizer->Add( buttonAndLabel.label, 0, wxALIGN_CENTER );
  sizer->Add( oneButtonSizer );
  return buttonAndLabel;
}

class ResizeDialogImpl : public wxDialog{
public:
  ResizeDialogImpl( wxWindow* parent, const wxString& title, const ResizeDialogSettings& );
  void SetBgColor(const faint::DrawSource&);

  // GetSelection is valid only if ShowModal() returns wxID_OK
  ResizeDialogSettings GetSelection() const;
private:
  bool GoodValues() const;
  bool ChangedValues() const;
  void SetDefaultResizeOption( ResizeDialogSettings::ResizeType );
  void SetTypeAndClose(ResizeDialogSettings::ResizeType, ButtonWithLabel&);
  void UpdateProportions( bool ignoreFocus );
  void OnCopySize( wxCommandEvent& );
  void OnPasteSize( wxCommandEvent& );
  void OnValueUpdate( wxCommandEvent& );
  void OnToggleProportional( wxCommandEvent& );
  void OnToggleProportionalKB( wxCommandEvent& );
  void OnTopLeftResize( wxCommandEvent& );
  void OnCenterResize( wxCommandEvent& );
  void OnScale( wxCommandEvent& );
  ResizeDialogSettings::ResizeType m_resizeType;
  ValueTextControl* m_widthCtrl;
  ValueTextControl* m_heightCtrl;
  wxObject* m_lastChanged;
  wxCheckBox* m_proportional;
  ButtonWithLabel m_anchorTopLeft;
  ButtonWithLabel m_anchorCenter;
  ButtonWithLabel m_scale;
  wxButton* m_cancel;
  wxBitmap m_placementBmp;
  IntSize m_bmpSize;
  DECLARE_EVENT_TABLE()
};

int ID_TOP_LEFT = wxNewId();
int ID_CENTER = wxNewId();
int ID_SCALE = wxNewId();
int ID_TOGGLE_CHECK = wxNewId();
int ID_COPY_SIZE = wxNewId();
int ID_PASTE_SIZE = wxNewId();

ResizeDialogImpl::ResizeDialogImpl( wxWindow* parent, const wxString& title, const ResizeDialogSettings& settings )
  : wxDialog( parent, wxID_ANY, title),
    m_bmpSize(get_resize_icon_size())
{
  const ArtContainer& icons = wxGetApp().GetArtContainer();

  // The outermost vertical sizer
  wxBoxSizer* rows = new wxBoxSizer(wxVERTICAL);

  // A proportion checkbox
  wxBoxSizer* proportionRow = new wxBoxSizer( wxHORIZONTAL );
  m_proportional = new wxCheckBox( this, wxID_ANY, "&Proportional" );
  m_proportional->SetValue( settings.proportional );
  proportionRow->Add( m_proportional, 0 );
  rows->Add( proportionRow, 0, wxLEFT|wxRIGHT|wxUP, faint::panel_padding );
  rows->AddSpacer(faint::item_spacing);

  // The row with the width and height entry fields
  wxBoxSizer* entryRow = new wxBoxSizer( wxHORIZONTAL );
  m_widthCtrl = add_entry_field( this, entryRow, "&Width", settings.size.w );
  entryRow->AddSpacer(faint::item_spacing);
  m_lastChanged = m_widthCtrl;
  m_heightCtrl = add_entry_field( this, entryRow, "&Height", settings.size.h );
  rows->Add(entryRow, 0, wxLEFT|wxRIGHT, faint::panel_padding );
  rows->AddSpacer(faint::item_spacing);

  const wxBitmap& scaleBmp = icons.Get( Icon::RESIZEDLG_SCALE );

  // The buttons for resize type choice
  wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);
  m_placementBmp = icons.Get(Icon::RESIZEDLG_PLACEMENT);
  if ( !settings.scaleOnly ){
    m_anchorTopLeft = add_button( this, buttonRow, ID_TOP_LEFT,
      "Resize drawing area right and down (Key: Q)",
      to_wx_bmp(color_bitmap(faint::Color(255,255,255), m_bmpSize)));
    m_anchorCenter = add_button( this, buttonRow, ID_CENTER,
      "Resize drawing area in all directions (Key: W)",
      to_wx_bmp(color_bitmap(faint::Color(255,255,255), m_bmpSize)));
  }
  m_scale = add_button( this, buttonRow, ID_SCALE,
    "Scale the image (Key: E)",
    scaleBmp);

  m_cancel = new wxButton(this, wxID_CANCEL, wxEmptyString,
    wxDefaultPosition, to_wx(faint::big_button_size) );
  buttonRow->Add( m_cancel, 0 );
  rows->Add(buttonRow, 0, wxLEFT|wxRIGHT|wxDOWN, faint::panel_padding );

  // Initialize size and layout
  SetSizerAndFit(rows);

  m_widthCtrl->SetFocus();
  m_resizeType = ResizeDialogSettings::UNKNOWN;
  if ( settings.scaleOnly ){
    SetDefaultResizeOption( ResizeDialogSettings::RESCALE );
    // There's no reason to indicate the default when only one option
    // is available
    m_scale.HideLabel();
  }
  else {
    SetDefaultResizeOption( settings.defaultButton );
  }

  faint::Accelerators acc;
  // Redundantly add Alt+P as an accelerator, to prevent the check-box
  // from stealing the focus from the entry fields
  acc.Add( wxACCEL_ALT, (int)'P', ID_TOGGLE_CHECK );

  // Since P is unused in the dialog, allow using P without modifier
  // to toggle the check-box
  acc.Add( wxACCEL_NORMAL, (int)'P', ID_TOGGLE_CHECK );

  // Each resize option
  acc.Add( wxACCEL_NORMAL, (int)'Q', ID_TOP_LEFT );
  acc.Add( wxACCEL_NORMAL, (int)'W', ID_CENTER );
  acc.Add( wxACCEL_NORMAL, (int)'E', ID_SCALE );

  // Alt+C copies both the width and height
  acc.Add( wxACCEL_ALT, (int)'C', ID_COPY_SIZE );
  acc.Add( wxACCEL_ALT, (int)'V', ID_PASTE_SIZE );
  SetAcceleratorTable(acc.Get());

  // Center on parent
  Centre(wxBOTH);
}

void ResizeDialogImpl::SetBgColor( const faint::DrawSource& src ){
  if ( m_anchorTopLeft.Exists() ){
    // Update the background color of the resize-buttons
    m_anchorTopLeft.button->SetBitmap( create_resize_bitmap( src, m_bmpSize, m_placementBmp, wxPoint(0,0) ) );
    m_anchorCenter.button->SetBitmap( create_resize_bitmap( src, m_bmpSize, m_placementBmp, wxPoint(16,16) ) );
  }
}

ResizeDialogSettings ResizeDialogImpl::GetSelection() const{
  return ResizeDialogSettings( IntSize(m_widthCtrl->GetValue(), m_heightCtrl->GetValue()), m_proportional->GetValue(), false, m_resizeType );
}

void ResizeDialogImpl::SetDefaultResizeOption( ResizeDialogSettings::ResizeType type ){

  m_scale.HideLabel();
  m_anchorCenter.HideLabel();
  m_anchorTopLeft.HideLabel();

  if ( type == ResizeDialogSettings::RESCALE ){
    SetDefaultItem( m_scale.button );
    m_scale.ShowLabel();
  }
  else if ( type == ResizeDialogSettings::RESIZE_CENTER ){
    SetDefaultItem( m_anchorCenter.button );
    m_anchorCenter.ShowLabel();
  }
  else {
    SetDefaultItem( m_anchorTopLeft.button );
    m_anchorTopLeft.ShowLabel();
  }
}

void ResizeDialogImpl::OnValueUpdate( wxCommandEvent& event ){
  m_lastChanged = event.GetEventObject();
  UpdateProportions( true );
}

void ResizeDialogImpl::OnToggleProportional( wxCommandEvent& ){
  UpdateProportions( false );
}

void ResizeDialogImpl::OnTopLeftResize( wxCommandEvent& ){
  SetTypeAndClose(ResizeDialogSettings::RESIZE_TOP_LEFT,
    m_anchorTopLeft);
}

void ResizeDialogImpl::OnCenterResize( wxCommandEvent& ){
  SetTypeAndClose(ResizeDialogSettings::RESIZE_CENTER,
    m_anchorCenter);
}

void ResizeDialogImpl::SetTypeAndClose( ResizeDialogSettings::ResizeType resizeType, ButtonWithLabel& btn ){
  btn.SetFocus();
  m_resizeType = resizeType;

  EndModal( GoodValues() && ChangedValues() ?
    wxID_OK : wxID_CANCEL );
}


void ResizeDialogImpl::OnCopySize( wxCommandEvent& ){
  faint::Clipboard clip;
  if ( clip.Good() ){
    IntSize size(m_widthCtrl->GetValue(), m_heightCtrl->GetValue());
    clip.SetText(serialize_size(size));
  }
}

void ResizeDialogImpl::OnPasteSize( wxCommandEvent& ){
  faint::Clipboard clip;
  if ( !clip.Good() ){
    return;
  }
  faint::utf8_string str;
  if ( !clip.GetText(str) ){
    return;
  }

  Optional<IntSize> maybeSize = deserialize_size(str);
  if ( maybeSize.IsSet() ){
    // Disable proportional, since both values should be kept
    m_proportional->SetValue(false);
    const IntSize& size(maybeSize.Get());
    m_widthCtrl->SetValue(size.w);
    m_heightCtrl->SetValue(size.h);
  }
}

void ResizeDialogImpl::OnScale( wxCommandEvent& ){
  SetTypeAndClose(ResizeDialogSettings::RESCALE,
    m_scale);
}

bool ResizeDialogImpl::GoodValues() const{
  return m_widthCtrl->GetValue() > 0 &&
    m_heightCtrl->GetValue() > 0;
}

bool ResizeDialogImpl::ChangedValues() const{
  return m_widthCtrl->GetValue() != m_widthCtrl->GetOldValue() ||
    m_heightCtrl->GetValue() != m_heightCtrl->GetOldValue();
}

void ResizeDialogImpl::OnToggleProportionalKB( wxCommandEvent& ){
  m_proportional->SetValue( !m_proportional->GetValue() );
  UpdateProportions(false);
}

void ResizeDialogImpl::UpdateProportions( bool ignoreFocus ){
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
      set_from_ratio( *m_heightCtrl, get_change_ratio(*m_widthCtrl) );
    }

    else {
      set_from_ratio( *m_widthCtrl, get_change_ratio(*m_heightCtrl) );
    }
  }
}

BEGIN_EVENT_TABLE(ResizeDialogImpl, wxDialog)
EVT_COMMAND(-1, EVT_VALUE_TEXT_CONTROL_UPDATE, ResizeDialogImpl::OnValueUpdate)
EVT_CHECKBOX(-1, ResizeDialogImpl::OnToggleProportional)
EVT_MENU(ID_TOGGLE_CHECK, ResizeDialogImpl::OnToggleProportionalKB)
EVT_BUTTON(ID_TOP_LEFT, ResizeDialogImpl::OnTopLeftResize)
EVT_BUTTON(ID_CENTER, ResizeDialogImpl::OnCenterResize)
EVT_BUTTON(ID_SCALE, ResizeDialogImpl::OnScale)
EVT_MENU(ID_COPY_SIZE, ResizeDialogImpl::OnCopySize)
EVT_MENU(ID_PASTE_SIZE, ResizeDialogImpl::OnPasteSize)
END_EVENT_TABLE()

ResizeDialog::ResizeDialog(){
}

bool ResizeDialog::ShowModal(wxWindow* parent, DialogFeedback& feedback){
  CanvasInterface& canvas = feedback.GetCanvas();
  // Fixme: Some of this should be updated to use IntSize etc. as
  // early (code-wise) as possible. Note that only objects should
  // support floating scaling.

  ApplyTarget target(get_apply_target(canvas));
  // Fixme: Floating point size needed for objects
  AppContext& app = GetAppContext();
  ResizeDialogImpl resizeDialog( parent,
    get_resize_dialog_title(canvas, target),
    modified( app.GetDefaultResizeDialogSettings(),
      floored(get_apply_target_size(canvas, target)),// Fixme: Floor bad for objects
      get_scale_only(target) ) );

  resizeDialog.SetBgColor( app.Get( ts_BgCol ) );
  if ( faint::show_modal( resizeDialog ) != wxID_OK ){
    return false;
  }

  ResizeDialogSettings s = resizeDialog.GetSelection();
  if ( s.size.w <= 0 || s.size.h <= 0 ){
    // Invalid size specified, do nothing.
    return false;
  }

  app.SetDefaultResizeDialogSettings(s);

  if ( target == APPLY_RASTER_SELECTION ){
    m_command.Set(get_scale_raster_selection_command( canvas.GetImage(), s.size ));
    return true;
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    // Fixme: Should already be floated.
    m_command.Set(context_scale_objects(canvas, floated(s.size)));
    return true;
  }

  // Scale or resize the entire image
  if ( s.defaultButton == ResizeDialogSettings::RESIZE_TOP_LEFT ){
    m_command.Set(get_resize_command( canvas.GetBitmap(),
        rect_from_size(s.size), GetAppContext().Get(ts_BgCol) ) );
  }
  else if ( s.defaultButton == ResizeDialogSettings::RESIZE_CENTER ){
    m_command.Set(get_resize_command( canvas.GetBitmap(),
        centered_resize_rect(s.size, canvas.GetSize()),
        GetAppContext().Get(ts_BgCol) ));
  }
  else if ( s.defaultButton == ResizeDialogSettings::RESCALE ){
    m_command.Set(get_rescale_command(s.size, ScaleQuality::BILINEAR));
  }
  return true;
}

Command* ResizeDialog::GetCommand(){
  return m_command.Retrieve();
}
