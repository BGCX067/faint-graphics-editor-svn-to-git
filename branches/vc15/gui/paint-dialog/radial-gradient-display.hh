namespace faint{

class RadialGradientDisplay : public wxPanel {
public:
  RadialGradientDisplay(wxWindow* parent,
    const wxSize& size,
    DialogContext& dialogContext)
    : wxPanel(parent, wxID_ANY)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Prevent flicker on full refresh
    SetInitialSize(size);

    m_slider = new RadialSlider(this,
      wxSize(size.GetWidth(), g_sliderHeight),
      m_gradient,
      dialogContext);
    m_slider->SetPosition(wxPoint(0, size.GetHeight() - g_sliderHeight)); // Fixme

    Bind(EVT_GRADIENT_SLIDER_CHANGE,
      [this](wxEvent&){
        UpdateBitmap();
        Refresh();
      });

    Bind(wxEVT_PAINT,
      [this](wxPaintEvent& event){
        wxPaintDC dc(this);
        dc.DrawBitmap(m_bmp, 0, 0);
        event.Skip();
      });
  }

  RadialGradient GetGradient() const{
    return m_gradient;
  }

  void SetGradient(const RadialGradient& g){
    m_gradient = g;
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  void SetStops(const color_stops_t& stops){
    m_gradient.SetStops(stops);
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  bool SetBackgroundColour(const wxColour& bgColor) override{
    wxPanel::SetBackgroundColour(bgColor);
    m_slider->SetBackgroundColour(bgColor);
    UpdateBitmap();
    return true;
  }
private:
  void UpdateBitmap(){
    IntSize sz(to_faint(GetSize()));
    Bitmap bg(sz - IntSize(0, g_sliderHeight), to_faint(GetBackgroundColour()));

    IntSize gSz(sz - IntSize(10, g_sliderHeight));
    m_gradientBmp = subbitmap(gradient_bitmap(Gradient(m_gradient), IntSize(gSz.w * 2, gSz.h)),
      IntRect(IntPoint(gSz.w,0), gSz));
    blit(offsat(with_border(m_gradientBmp), {5,0}), onto(bg));
    m_bmp = to_wx_bmp(bg);
  }

  wxBitmap m_bmp;
  RadialGradient m_gradient;
  Bitmap m_gradientBmp;
  RadialSlider* m_slider;
};

} // namespace
