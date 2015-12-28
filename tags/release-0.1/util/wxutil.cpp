#include "wx/wx.h"
#include "wx/mstream.h"
#include "wx/fontenum.h"
#include "wx/filename.h"
#include "wx/stdpaths.h"
#include "util.hh"
#include "colorutil.hh" // For ColorToolTip
#include "convertwx.hh"

wxBitmap from_any( const char* data, size_t len, wxBitmapType type ){
  wxMemoryInputStream stream(data, len);
  wxImage image( stream, type );
  if ( !image.IsOk() ){
    return wxBitmap(10,10);
  }
  if ( image.HasMask() ){
    image.InitAlpha();
  }
  return wxBitmap(image);
}

faint::Bitmap from_jpg( const char* jpg, size_t len ){
  return to_faint(from_any( jpg, len, wxBITMAP_TYPE_JPEG ));
}

faint::Bitmap from_png( const char* png, size_t len ){
  return to_faint(from_any( png, len, wxBITMAP_TYPE_PNG ));
}

std::string to_png( const faint::Bitmap& bmp ){
  wxBitmap bmp_wx( to_wx( bmp ) );
  wxImage img_wx( bmp_wx.ConvertToImage() );
  wxMemoryOutputStream stream;
  img_wx.SaveFile( stream, wxBITMAP_TYPE_PNG );
  size_t len = stream.GetSize();
  char* buf = new char[len];
  stream.CopyTo(buf, len);
  std::string ret(buf, len);
  delete[] buf;
  return ret;
}

int python_bind_modifiers( const wxKeyEvent& event ){
  int modifiers = 0;
  modifiers |= event.ControlDown() ? 1 : 0;
  modifiers |= event.ShiftDown() ? 2 : 0;
  modifiers |= event.AltDown() ? 4 : 0;
  return modifiers;
}

std::vector<std::string> available_font_facenames(){
  std::vector<std::string> v;
  wxFontEnumerator enumerate;
  wxArrayString faceNames = enumerate.GetFacenames();
  for ( size_t i = 0; i != faceNames.size(); i++ ){
    v.push_back(std::string(faceNames[i]));
  }
  return v;
}

bool ValidFacename( const std::string& name ){
  return wxFontEnumerator::IsValidFacename( name );
}

bool ValidSaveFileName( const std::string& path ){
  wxFileName filename( path );
  return !filename.IsDir() && filename.IsAbsolute();
}

faint::Color GetHighlightColor(){
  return to_faint(wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ));
}

std::string GetHomeDir(){
  return std::string( wxGetHomeDir() );
}

wxFileName GetFaintDir(){
  wxStandardPathsBase& paths = wxStandardPaths::Get();
  wxString exePath = paths.GetExecutablePath();
  wxFileName exeFile(exePath);
  return exeFile.GetPath();
}

std::string GetDataDir(){
  wxFileName exeDir = GetFaintDir();
  if ( exeDir == "/usr/bin" ){
    return "usr/share/faint";
  }
  else{
    wxString path( exeDir.GetFullPath() );
    path.Replace("\\", "/");
    return std::string(path);
  }
}

bool FileExists( const std::string& path ){
  return wxFileName( path ).FileExists();
}

std::string GetFullPath( const std::string& path ){
  return std::string(wxFileName( path ).GetFullPath());
}

std::string JoinPath( const std::string& a, const std::string& b ){
  wxString p1(a);
  wxString p2(b);
  return std::string(p1 + wxFileName::GetPathSeparator() + p2);
}

wxBitmap* ColorBitmap( const faint::Color& color, int w, int h, bool border ){
  int r1 = ( color.r * color.a + 192 * ( 255 - color.a ) ) / 255;
  int g1 = ( color.g * color.a + 192 * ( 255 - color.a ) ) / 255;
  int b1 = ( color.b * color.a + 192 * ( 255 - color.a ) ) / 255;

  int r2 = ( color.r * color.a + 255 * ( 255 - color.a ) ) / 255;
  int g2 = ( color.g * color.a + 255 * ( 255 - color.a ) ) / 255;
  int b2 = ( color.b * color.a + 255 * ( 255 - color.a ) ) / 255;

  wxBitmap* bmp = new wxBitmap( w, h );
  {
    wxMemoryDC dc( *bmp );
    dc.SetBackground( wxBrush( wxColour( r2, g2, b2 ) ) );
    dc.Clear();
    dc.SetBrush( wxBrush( wxColour( r1, g1, b1 ) ) );
    dc.SetPen( wxPen( wxColour( r1, g1, b1 ) ) );
    dc.DrawRectangle(0,0, w/2, h/2);
    dc.DrawRectangle(w/2,h/2,w/2,h/2);

    if ( border ){
      dc.SetBrush( *wxTRANSPARENT_BRUSH );
      dc.SetPen( wxPen( wxColour(0, 0, 0 )) );
      dc.DrawRectangle( 0, 0, w, h );
    }
  }
  return bmp;
}
