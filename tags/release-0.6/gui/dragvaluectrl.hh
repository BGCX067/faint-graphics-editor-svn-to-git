#ifndef FAINT_DRAGVALUECTRL_HH
#define FAINT_DRAGVALUECTRL_HH
#include "wx/wx.h"
#include "geo/point.hh"
#include "geo/range.hh"
#include "util/guiutil.hh"
DECLARE_EVENT_TYPE(EVT_DRAG_VALUE_CHANGE, -1);

class DragValueCtrl : public wxPanel {
public:
  // Note: Does not use tooltips as they tended to obscure the value
  DragValueCtrl(wxWindow* parent, const IntRange&, const description_t&);
  void SetValue(int);
private:
  void OnCaptureLost(wxMouseCaptureLostEvent&);
  void OnLeftDown(wxMouseEvent&);
  void OnLeftUp(wxMouseEvent&);
  void OnMotion(wxMouseEvent&);
  void OnMouseEnter(wxMouseEvent&);
  void OnMouseLeave(wxMouseEvent&);
  void OnPaint(wxPaintEvent&);
  void SendChangeEvent( int );
  Point m_current;
  int m_currentValue;
  Point m_origin;
  int m_originValue;
  IntRange m_range;
  std::string m_statusText;
  DECLARE_EVENT_TABLE()
};

#endif
