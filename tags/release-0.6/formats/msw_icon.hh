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
#ifndef FAINT_MSW_ICON
#define FAINT_MSW_ICON
#include "saveresult.hh"

class ImageProps;
namespace faint{
  void load_icon( const std::string& filename, ImageProps& imageProps );
  SaveResult save_icon( const std::string& filename, const std::vector<const faint::Bitmap>& );
}

#endif
