#ifndef FAINT_RESIZE_DIALOG
#define FAINT_RESIZE_DIALOG
#include "wx/dialog.h"
#include "wx/bmpbuttn.h"
#include "resizedialogsettings.hh"
class ValueTextControl;
class wxCheckBox;
class ArtContainer;
class wxStaticText;

class ButtonWithLabel{
public:
  ButtonWithLabel();
  wxStaticText* label;
  wxBitmapButton* button;
};

class ResizeDialog : public wxDialog{
public:
  ResizeDialog( wxWindow* parent, const wxString& title, const ResizeDialogSettings&, const ArtContainer& art );

  // GetSelection is valid only if ShowModal() returns wxID_OK
  ResizeDialogSettings GetSelection() const;

private:
  bool GoodValues() const;
  bool ChangedValues() const;
  void SetDefaultResizeOption( ResizeDialogSettings::ResizeType );
  void UpdateProportions( bool ignoreFocus );
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
  DECLARE_EVENT_TABLE()
};
#endif
