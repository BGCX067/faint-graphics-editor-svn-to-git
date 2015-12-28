#ifndef FAINT_RESIZE_DIALOG
#define FAINT_RESIZE_DIALOG
#include "gui/command-dialog.hh"

class ResizeDialog : public CommandDialog{
public:
  ResizeDialog();
  Command* GetCommand() override;
  bool ShowModal(wxWindow*, DialogFeedback&) override;
private:
  ResizeDialog(const ResizeDialog&);
  PendingCommand m_command;
};

#endif
