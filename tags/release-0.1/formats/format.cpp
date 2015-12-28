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

#include "format.hh"
#include "objects/object.hh"
#include "faintdc.hh"

Format::Format( const std::string& format_, const std::string& label_, bool canSave_, bool canLoad_ )
  : format(format_), label(label_), canSave(canSave_), canLoad(canLoad_)
{}

const std::string& Format::GetFormat() const{
  return format;
}

const std::string& Format::GetLabel() const{
  return label;
}

bool Format::CanLoad(){
  return canLoad;
}

bool Format::CanSave(){
  return canSave;
}
