// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#include <functional>
#include <algorithm>
#include <iterator>
#include "formats/file-formats.hh"
#include "text/formatting.hh"
#include "util/file-format-util.hh"
#include "util/iter.hh"

namespace faint{

std::vector<Format*> built_in_file_formats(){
  return {
    format_wx_png(),
    format_gif(),
    format_ico(),
    format_cur(),
    // Fixme: Consider using FormatBMP instead of FormatWX for 24 bit as well
    format_wx_bmp_24bit(),
    format_bmp(BitmapQuality::GRAY_8BIT),
    format_bmp(BitmapQuality::COLOR_8BIT),
    format_wx_jpg()};
}

template<typename T>
T select(const T& src, const std::function<bool(const typename T::value_type&)>& func){
  T dst;
  std::copy_if(src.begin(), src.end(), back_inserter(dst), func);
  return dst;
}

std::vector<Format*> loading_file_formats(const std::vector<Format*>& allFormats){
  return select(allFormats,
    {[](const Format* f){return f->CanLoad();}}); // Fixme: Are the outer braces required? gcc4.8.1 wouldn't compile without.
}

std::vector<Format*> saving_file_formats(const std::vector<Format*>& allFormats){
  return select(allFormats,
    {[](const Format* f){return f->CanSave();}}); // Fixme: Are the outer braces required? gcc4.8.1 wouldn't compile without.
}

static utf8_string wildcarded(const FileExtension& ext){
  return utf8_string("*.") + ext.Str();
}

static utf8_string extensions_filter(const std::vector<FileExtension>& extensions){
  return join_fn(utf8_string(";"), extensions, {wildcarded}); // Fixme: Are the outer braces required? gcc4.8.1 wouldn't compile without.
}

utf8_string file_dialog_filter(const std::vector<Format*>& formats){
  auto single_format_filter = [](const Format* f){
    return f->GetLabel() + utf8_string("|") + extensions_filter(f->GetExtensions());
  };

  return join_fn(utf8_string("|"), formats, {single_format_filter});
}

int get_file_format_index(const std::vector<Format*>& formats, const FileExtension& extension){
  for (size_t i = 0; i != formats.size(); i++){
    if (formats[i]->Match(extension)){
      return resigned(i);
    }
  }
  return -1;
}

utf8_string combined_file_dialog_filter(const utf8_string& description, const std::vector<Format*>& formats){
  if (formats.empty()){
    return utf8_string("");
  }

  auto format_to_extensions_filter = [](const Format* const& f){
    return extensions_filter(f->GetExtensions());
  };

  return join(utf8_string("|"), description,
    join_fn(utf8_string(";"), formats, {format_to_extensions_filter}),  // Fixme: Are the outer braces required? gcc4.8.1 wouldn't compile without.
    file_dialog_filter(formats),
    utf8_string("All files"),
    utf8_string("*.*"));
}

Format* get_load_format(const std::vector<Format*>& formats, const FileExtension& ext){
  for (Format* f : formats){
    if (f->Match(ext) && f->CanSave()){
      return f;
    }
  }
  return nullptr;
}

Format* get_save_format(const std::vector<Format*>& formats, const FileExtension& ext){
  for (Format* format : formats){
    if (format->Match(ext) && format->CanSave()){
      return format;
    }
  }
  return nullptr;
}

Format* get_save_format(const std::vector<Format*>& formats, const FileExtension& ext, int filterIndex){

  if (0 < filterIndex && filterIndex < resigned(formats.size())){
    Format* f = formats[to_size_t(filterIndex)];
    if (f->Match(ext)){
      return f;
    }
  }
  return get_save_format(formats, ext);
}

bool has_save_format(const std::vector<Format*>& formats, const FileExtension& ext){
  return end(formats) != std::find_if(begin(formats), end(formats),
    [&](Format* f){ return f->CanSave() && f->Match(ext); });
}

}
