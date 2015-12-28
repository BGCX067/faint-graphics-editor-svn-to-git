// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include "bitmap/bitmap.hh"
#include "convertwx.hh"
#include "geo/canvasgeo.hh"
#include "geo/point.hh"
#include "util/color.hh"
#include "util/colorutil.hh"
#include "util/pathutil.hh"
#include "util/util.hh"
#include "wx/filename.h"
#include "wx/fontenum.h"
#include "wx/mstream.h"
#include "wx/stdpaths.h"
#include "wx/wx.h"

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

std::string to_png_string( const faint::Bitmap& bmp ){
  wxImage img_wx(to_wx_image(bmp));
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

wxFont GetDefaultFont(){
  return wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false );
}

std::string get_default_font_name(){
  static std::string defaultName(GetDefaultFont().GetFaceName());
  return defaultName;
}

int get_default_font_size(){
  static int defaultSize(GetDefaultFont().GetPointSize());
  return defaultSize;
}

bool valid_facename( const std::string& name ){
  return wxFontEnumerator::IsValidFacename( name );
}

faint::Color get_highlight_color(){
  return to_faint(wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ));
}

std::string get_home_dir(){
  return std::string( wxGetHomeDir() );
}

wxFileName get_faint_dir(){
  wxStandardPathsBase& paths = wxStandardPaths::Get();
  wxString exePath = paths.GetExecutablePath();
  wxFileName exeFile(exePath);
  return exeFile.GetPath();
}

std::string get_data_dir(){
  wxFileName exeDir = get_faint_dir();
  if ( exeDir == "/usr/bin" ){
    return "usr/share/faint";
  }
  else{
    wxString path( exeDir.GetFullPath() );
    path.Replace("\\", "/");
    return std::string(path);
  }
}

std::string get_help_dir(){
  return join_path(get_data_dir(), "help");
}

bool file_exists( const std::string& path ){
  return wxFileName( path ).FileExists();
}

std::string get_full_path( const std::string& path ){
  return std::string(wxFileName( path ).GetFullPath());
}

std::string join_path( const std::string& a, const std::string& b ){
  wxString p1(a);
  wxString p2(b);
  return std::string(p1 + wxFileName::GetPathSeparator() + p2);
}

std::string convert_wide_to_utf8( const wchar_t& c ){
  wxString wxStr;
  wxStr += c;
  return std::string(wxStr.ToUTF8());
}


namespace faint{
  namespace mouse {
    IntPoint screen_position(){
      return to_faint(wxGetMousePosition());
    }

    IntPoint view_position( const wxWindow& w ){
      return to_faint( w.ScreenToClient( wxGetMousePosition() ) );
    }

    Point image_position( const Geometry& g, const wxWindow& w ){
      Point p( floated(view_position(w)) );
      const faint::coord zoom = g.zoom.GetScaleFactor();
      return Point( ( p.x + g.x0 - g.left_border ) / zoom,
        ( p.y + g.y0 - g.top_border ) / zoom );
    }
  }
}
