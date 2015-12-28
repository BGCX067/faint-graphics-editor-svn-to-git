#ifndef FAINT_RESIZEDIALOGSETTINGS_HH
#define FAINT_RESIZEDIALOGSETTINGS_HH

class ResizeDialogSettings{
public:
  enum ResizeType{RESCALE, RESIZE_TOP_LEFT, RESIZE_CENTER, UNKNOWN };
  ResizeDialogSettings( int w_=0, int h_=0, bool proportional_=false, bool scaleOnly_ = false, ResizeType defaultButton_=RESIZE_TOP_LEFT)
    : proportional( proportional_ ),
      scaleOnly( scaleOnly_ ),
      w(w_),
      h(h_),
      defaultButton( defaultButton_ )
  {}
  ResizeDialogSettings( size_t w_, size_t h_, bool scaleOnly_, const ResizeDialogSettings& other ){
    *this = other;
    w = w_;
    h = h_;
    scaleOnly = scaleOnly_;
  }

  bool proportional;
  bool scaleOnly;
  int w;
  int h;
  ResizeType defaultButton;
};

#endif
