#include "wx/wx.h"
#include "wx/bookctrl.h"
#include "gui/dialog-context.hh"
#include "util/status-interface.hh"

#define GUI_TEST_FUNCTION(NAME)void NAME(wxWindow*, faint::StatusInterface&, faint::DialogContext&)

GUI_TEST_FUNCTION(gui_test_bitmap_list_ctrl);
GUI_TEST_FUNCTION(gui_test_palette_ctrl);
GUI_TEST_FUNCTION(gui_test_selected_color_ctrl);
GUI_TEST_FUNCTION(gui_test_slider);
GUI_TEST_FUNCTION(gui_test_static_bitmap);

using test_init_func_t = void(*)(wxWindow*,
  faint::StatusInterface&,
  faint::DialogContext&);

void add_test(const wxString& name,
  test_init_func_t f,
  wxBookCtrlBase* pageList,
  faint::StatusInterface& statusInterface,
  faint::DialogContext& dialogContext)
{
  auto* p = new wxPanel(pageList);
  f(p, statusInterface, dialogContext);
  pageList->AddPage(p, name);
}

void add_gui_tests(wxBookCtrlBase* pageList,
  faint::StatusInterface& statusInterface,
  faint::DialogContext& dialogContext)
{

  add_test("BitmapListCtrl", gui_test_bitmap_list_ctrl,
    pageList, statusInterface, dialogContext);

  add_test("PaletteCtrl", gui_test_palette_ctrl,
    pageList, statusInterface, dialogContext);

  add_test("SelectedColorCtrl", gui_test_selected_color_ctrl,
    pageList, statusInterface, dialogContext);

  add_test("Slider", gui_test_slider,
    pageList, statusInterface, dialogContext);

  add_test("StaticBitmap", gui_test_static_bitmap,
    pageList, statusInterface, dialogContext);


}
