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

#include <sstream>
#include "wx/animate.h"
#include "wx/filename.h"
#include "wx/log.h"
#include "formats/format_wx.hh"
#include "util/canvasinterface.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/imageprops.hh"
#include "util/imageutil.hh"

class ScopedErrorLog{
// Replaces the wxWidgets error logging within a scope.
// This prevents the wxWidgets log dialog from appearing, so that the messages
// can be displayed in a different way. Also removes timestamps.
public:
  ScopedErrorLog()
    : m_stream(""),
      m_logTarget(&m_stream),
      m_prevTarget(wxLog::SetActiveTarget(&m_logTarget)),
      m_prevTimeStamp(wxLog::GetTimestamp())
  {
    wxLog::SetTimestamp("");
  }
  ~ScopedErrorLog(){
    // Restore normal logging on destruction
    wxLog::SetActiveTarget(m_prevTarget);
    wxLog::SetTimestamp(m_prevTimeStamp);
  }
  std::string GetMessages() const {
    return m_stream.str();
  }
private:
  ScopedErrorLog( const ScopedErrorLog& ); // Prevent copy
  ScopedErrorLog& operator=(const ScopedErrorLog& ); // Prevent assignment
  std::stringstream m_stream;
  wxLogStream m_logTarget;
  wxLog* m_prevTarget;
  wxString m_prevTimeStamp;
};

static bool convert_append(wxImage& image, ImageProps& props, const delay_t& delay ){
  if ( !image.IsOk() ){
    return false;
  }
  if ( image.HasMask() && !image.HasAlpha() ){
    image.InitAlpha();
  }
  wxBitmap bmp( image );
  props.AddFrame( to_faint( bmp ), FrameInfo(delay) );
  return true;
}

namespace faint{

bool load_gif_wx( const FilePath& filePath, ImageProps& props ){
  wxAnimation animation( filePath.ToWx().GetLongPath(), wxANIMATION_TYPE_GIF );
  if ( !animation.IsOk() ){
    return false;
  }
  const size_t numFrames = animation.GetFrameCount();
  if ( numFrames == 0 ){
    return false;
  }

  for ( size_t i = 0; i != numFrames; i++ ){
    wxImage img(animation.GetFrame(i));
    delay_t delay(animation.GetDelay(i));
    bool ok = convert_append(img, props, delay);
    if ( !ok ){
      return false;
    }
  }
  return true;
}

void load_file_wx( const FilePath& filePath, wxBitmapType bmpType, ImageProps& props ){
  ScopedErrorLog logger; // Collect wxWidgets log errors

  if ( bmpType == wxBITMAP_TYPE_GIF ){
    bool ok = load_gif_wx(filePath, props);
    if ( !ok ){
      props.SetError(logger.GetMessages());
    }
    return;
  }
  else {
    // I must load like this (i.e. create a wxImage, init alpha and
    // convert to Bitmap) to retain alpha channel. See wxWidgets ticket
    // #3019
    wxImage wxImg( filePath.ToWx().GetLongPath(), bmpType );
    bool ok = convert_append(wxImg, props, delay_t(0));
    if ( !ok ){
      props.SetError(logger.GetMessages());
    }
  }
}
} // namespace

FormatWX::FormatWX( const extension_t& ext, const label_t& label, wxBitmapType type )
  : Format( ext, label, can_save(true), can_load(true) ),
    m_bmpType(type)
{}

FormatWX::FormatWX( const std::vector<extension_t>& exts, const label_t& label, wxBitmapType type )
  : Format( exts, label, can_save(true), can_load(true) ),
    m_bmpType(type)
{}

void FormatWX::Load( const faint::FilePath& filePath, ImageProps& imageProps ){
  faint::load_file_wx(filePath, m_bmpType, imageProps);
}

SaveResult FormatWX::Save( const faint::FilePath& filePath, CanvasInterface& canvas ){
  const faint::Image& image(canvas.GetImage());
  faint::Bitmap bmp(faint::flatten(image));
  wxImage imageWx( to_wx_image( bmp ) );
  ScopedErrorLog logger;
  bool saveOk = imageWx.SaveFile( filePath.ToWx().GetLongPath(), m_bmpType );

  if ( !saveOk ){
    return SaveResult::SaveFailed(endline_sep(
      space_sep("Failed saving as", quoted(GetLabel()) + "."),
      logger.GetMessages()));
  }
  return SaveResult::SaveSuccessful();
}
