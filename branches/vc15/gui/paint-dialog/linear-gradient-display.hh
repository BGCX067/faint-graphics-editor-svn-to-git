#include "util-wx/make-event.hh"

namespace faint{

MAKE_FAINT_COMMAND_EVENT(GRADIENT_ANGLE_PICKED);

static Bitmap with_angle_indicator(const Bitmap& src, const Angle& angle){
  IntSize sz(src.GetSize());
  int dx = static_cast<int>(cos(-angle) * sz.w);
  int dy = static_cast<int>(sin(-angle) * sz.w);
  int cx = sz.w / 2;
  int cy = sz.h / 2;
  Bitmap dst(src);
  Color border(50,50,50);
  Color fill(255,255,255);

  IntPoint start(cx, cy);
  IntPoint end(cx + dx, cy + dy);
  draw_line(dst, {start, end}, {border, 3, LineStyle::SOLID, LineCap::BUTT});
  draw_line(dst, {start, end}, {fill, 1, LineStyle::SOLID, LineCap::BUTT});
  put_pixel(dst, start, border);
  return dst;
}

class LinearGradientDisplay : public wxPanel {
public:
  LinearGradientDisplay(wxWindow* parent, const wxSize& size,
    DialogContext& dialogContext)
    : wxPanel(parent, wxID_ANY),
      m_dialogContext(dialogContext),
      m_mouse(this),
      m_offset(5,0)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Prevent flicker on full refresh
    SetInitialSize(size);
    m_slider = new LinearSlider(this, wxSize(size.GetWidth(), g_sliderHeight),
      m_gradient, dialogContext);
    m_slider->SetPosition(wxPoint(0, size.GetHeight() - g_sliderHeight));
    SetCursor(get_art_container().Get(Cursor::CROSSHAIR));

    Bind(wxEVT_PAINT,
      [this](wxPaintEvent&){
        wxPaintDC dc(this);
        dc.DrawBitmap(m_bmp, 0, 0);
      });

    Bind(wxEVT_LEFT_DOWN,
      [this](wxMouseEvent& event){
        int max_x = m_bmp.GetWidth();
        int max_y = m_bmp.GetHeight();
        wxPoint pos(event.GetPosition());
        if (pos.x > max_x || pos.y > max_y){
          return;
        }
        m_mouse.Capture();
        SetAngleFromPos(GetBitmapPos(event));
      });

    Bind(wxEVT_LEFT_UP,
      [this](wxMouseEvent&){
        m_mouse.Release();
      });

    Bind(wxEVT_MOTION,
      [this](wxMouseEvent& event){
        if (m_mouse.HasCapture()){
          SetAngleFromPos(GetBitmapPos(event));
        }
      });

    Bind(EVT_GRADIENT_SLIDER_CHANGE,
      [this](wxEvent&){
        UpdateBitmap();
        Refresh();
      });
  }

  LinearGradient GetGradient() const{
    return m_gradient;
  }

  bool SetBackgroundColour(const wxColour& bgColor) override{
    wxPanel::SetBackgroundColour(bgColor);
    m_slider->SetBackgroundColour(bgColor);
    UpdateBitmap();
    return true;
  }

  void SetGradient(const LinearGradient& g){
    m_gradient = g;
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  void SetStops(const color_stops_t& stops){
    m_gradient.SetStops(stops);
    m_slider->UpdateGradient();
    UpdateBitmap();
    Refresh();
  }

private:
  IntPoint GetBitmapPos(const wxMouseEvent& event){
    const IntPoint pos(to_faint(event.GetPosition()) - m_offset);
    return event.ShiftDown() ? floored(adjust_to_45(floated(point_from_size(m_gradientBmp.GetSize() / 2)), floated(pos))) :
      pos;
  }

  void SetAngleFromPos(const IntPoint& mousePos){
    IntPoint pos = mousePos - point_from_size(m_gradientBmp.GetSize() / 2);
    if (pos.x == 0 && pos.y == 0){
      return;
    }
    Angle angle = -atan2(static_cast<double>(pos.y), static_cast<double>(pos.x));

    m_gradient.SetAngle(angle);
    UpdateBitmap();
    Refresh();
    wxCommandEvent event(EVT_GRADIENT_ANGLE_PICKED, GetId());
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
  }

  void UpdateBitmap(){
    IntSize sz(to_faint(GetSize()));
    Bitmap bg(sz, to_faint(GetBackgroundColour()));
    m_gradientBmp = gradient_bitmap(Gradient(unrotated(m_gradient)),
      sz - IntSize(10,g_sliderHeight)); // Fixme: Literals
    blit(offsat(with_border(with_angle_indicator(m_gradientBmp,
      m_gradient.GetAngle())), m_offset), onto(bg));
    m_bmp = to_wx_bmp(bg);
  }

  wxBitmap m_bmp;
  DialogContext& m_dialogContext;
  LinearGradient m_gradient;
  Bitmap m_gradientBmp;
  MouseCapture m_mouse;
  LinearSlider* m_slider;
  const IntPoint m_offset;
};

} // namespace
