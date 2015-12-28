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

#ifndef FAINT_IMAGEPROPS_HH
#define FAINT_IMAGEPROPS_HH
#include <string>
#include "util/frameprops.hh"

class ImageProps{
  // For creating an image, e.g. during loading.
public:
  ImageProps();
  ImageProps(const ImageInfo& firstFrame);
  ImageProps(const faint::Bitmap& firstBmp);
  ImageProps(const IntSize&, const objects_t&);
  ~ImageProps();
  FrameProps& AddFrame(const ImageInfo&);
  FrameProps& AddFrame( const faint::Bitmap&, const FrameInfo& );
  void AddWarning( const std::string& );
  std::string GetError() const;
  FrameProps& GetFrame( int num );
  size_t GetNumFrames() const;
  size_t GetNumWarnings() const;
  std::string GetWarning(size_t) const;
  bool IsBad() const;
  bool IsOk() const;
  void SetError( const std::string& ); // Fixme: use utf8_string to allow for example wide file names as part of the error string
private:
  std::string m_error;
  std::vector<FrameProps*> m_frames;
  bool m_ok;
  std::vector<std::string> m_warnings;
};

#endif
