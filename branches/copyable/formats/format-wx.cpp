// -*- coding: us-ascii-unix -*-
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

#include <map>
#include <sstream>
#include "wx/filename.h"
#include "wx/bitmap.h"
#include "wx/image.h"
#include "bitmap/bitmap.hh"
#include "formats/file-formats.hh"
#include "text/formatting.hh"
#include "util/canvas.hh"
#include "util/convert-wx.hh"
#include "util/image-props.hh"
#include "util/image-util.hh"
#include "util/scoped-error-log.hh"

namespace faint{
static bool convert_append(wxImage& image, ImageProps& props, const delay_t& delay){
  if (!image.IsOk()){
    return false;
  }
  if (image.HasMask() && !image.HasAlpha()){
    image.InitAlpha();
  }
  wxBitmap bmp(image);
  props.AddFrame(to_faint(bmp), FrameInfo(delay));
  return true;
}

void load_file_wx(const FilePath& filePath, wxBitmapType bmpType, ImageProps& props){
  assert(bmpType != wxBITMAP_TYPE_GIF);
  ScopedErrorLog logger; // Collect wxWidgets log errors
  // I must load like this (i.e. create a wxImage, init alpha and
  // convert to Bitmap) to retain alpha channel. See wxWidgets ticket
  // #3019
  wxImage wxImg(to_wx(filePath.Str()), bmpType);
  bool ok = convert_append(wxImg, props, delay_t(0));
  if (!ok){
    props.SetError(utf8_string(logger.GetMessages()));
  }
}

class FormatWX : public Format {
public:
  FormatWX(const FileExtension& ext, const label_t& label, wxBitmapType type)
    : Format(ext, label, can_save(true), can_load(true)),
      m_bmpType(type)
  {
    assert(type != wxBITMAP_TYPE_GIF);
  }

  FormatWX(const std::vector<FileExtension>& exts, const label_t& label, wxBitmapType type)
    : Format(exts, label, can_save(true), can_load(true)),
      m_bmpType(type)
  {
    assert(type != wxBITMAP_TYPE_GIF);
  }

  void Load(const FilePath& filePath, ImageProps& imageProps) override{
    load_file_wx(filePath, m_bmpType, imageProps);
  }

  void SetWxOption(const wxString& option, int value){
    m_intOptions[option] = value;
  }

  void SetWxOption(const wxString& option, const wxString& value){
    m_stringOptions[option] = value;
  }

  SaveResult Save(const FilePath& filePath, Canvas& canvas) override{
    const Image& image(canvas.GetImage());
    Bitmap bmp(flatten(image));
    wxImage imageWx(to_wx_image(bmp));

    for (auto option : m_intOptions){
      imageWx.SetOption(option.first, option.second);
    }

    for (auto option : m_stringOptions){
      imageWx.SetOption(option.first, option.second);
    }

    ScopedErrorLog logger;
    bool saveOk = imageWx.SaveFile(to_wx(filePath.Str()), m_bmpType);
    if (!saveOk){
      // Fixme: wxImage::SaveFile does not seem to reliably return false
      // on failure (Seen when attempting to overwrite locked .jpg on
      // Windows).
      return SaveResult::SaveFailed(utf8_string(endline_sep(
        space_sep("Failed saving as", quoted(GetLabel()) + "."),
        logger.GetMessages())));
    }
    return SaveResult::SaveSuccessful();
  }
private:
  wxBitmapType m_bmpType;
  std::map<wxString, int> m_intOptions;
  std::map<wxString, wxString> m_stringOptions;
};

Format* format_wx_bmp_24bit(){
  return new FormatWX(FileExtension("bmp"),
    label_t(utf8_string("Bitmap 24bit (*.bmp)")), wxBITMAP_TYPE_BMP);
}

// Fixme: Add assignment operator to FileExtension and get rid of this
template <typename T>
inline std::vector<T> vector_of(const T& e0, const T& e1){
  std::vector<T> v;
  v.push_back(e0);
  v.push_back(e1);
  return v;
}

Format* format_wx_jpg(){
  return new FormatWX(vector_of(FileExtension("jpg"), FileExtension("jpeg")), label_t(utf8_string("JPEG")), wxBITMAP_TYPE_JPEG);
}

Format* format_wx_png(){
  return new FormatWX(FileExtension("png"),
    label_t(utf8_string("Portable Network Graphics")), wxBITMAP_TYPE_PNG);
}

}
