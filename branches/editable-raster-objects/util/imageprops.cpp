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

#include "objects/object.hh"
#include "util/imageprops.hh"
#include "util/objutil.hh"

ImageProps::ImageProps()
  : m_ok(true)
{}

ImageProps::ImageProps(const ImageInfo& firstFrame)
  : m_ok(true)
{
  m_frames.push_back(new FrameProps(firstFrame));
}

ImageProps::ImageProps(const faint::Bitmap& firstBmp)
  : m_ok(true)
{
  m_frames.push_back(new FrameProps(firstBmp));
}

ImageProps::ImageProps(const IntSize& size, const objects_t& objects)
  : m_ok(true)
{
  m_frames.push_back(new FrameProps(size, objects));
}

ImageProps::~ImageProps(){
  for ( FrameProps* frame : m_frames ){
    delete frame;
  }
}

FrameProps& ImageProps::AddFrame(const ImageInfo& info ){
  FrameProps* frame = new FrameProps(info);
  m_frames.push_back(frame);
  return *frame;
}

FrameProps& ImageProps::AddFrame(const faint::Bitmap& bmp, const FrameInfo& info ){
  FrameProps* frame = new FrameProps(bmp, info);
  m_frames.push_back(frame);
  return *frame;
}

FrameProps& ImageProps::GetFrame( int in_num ){
  size_t num(to_size_t(in_num));
  assert(num < m_frames.size());
  return *m_frames[num];
}

size_t ImageProps::GetNumFrames() const{
  return m_frames.size();
}

size_t ImageProps::GetNumWarnings() const{
  return m_warnings.size();
}

std::string ImageProps::GetWarning(size_t num) const{
  assert(num < m_warnings.size());
  return m_warnings[num];
}
std::string ImageProps::GetError() const{
  assert(!m_ok);
  return m_error;
}

bool ImageProps::IsBad() const{
  return !IsOk();
}

bool ImageProps::IsOk() const{
  return m_ok && !m_frames.empty();
}

void ImageProps::SetError( const std::string& error ){
  m_error = error;
  m_ok = false;
}

void ImageProps::AddWarning( const std::string& warning ){
  m_warnings.push_back(warning);
}

