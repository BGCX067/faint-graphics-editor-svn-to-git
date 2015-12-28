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

#include <algorithm>
#include "formats/format.hh"
#include "objects/object.hh"
#include "rendering/faintdc.hh"
#include "util/formatting.hh"

Format::Format( const extension_t& ext, const label_t& label, const can_save& canSave, const can_load& canLoad ):
  m_canLoad(canLoad.Get()),
  m_canSave(canSave.Get()),
  m_label(label.Get())
{
  m_extensions.push_back(lowercase(ext.Get()));
}

Format::Format( const std::vector<extension_t>& extensions, const label_t& label, const can_save& canSave, const can_load& canLoad ):
  m_canLoad(canLoad.Get()),
  m_canSave(canSave.Get()),
  m_label(label.Get())
{
  for ( const extension_t& ext : extensions ){
    m_extensions.push_back(lowercase(ext.Get()));
  }
}

Format::~Format(){
}

bool Format::CanLoad() const{
  return m_canLoad;
}

bool Format::CanSave() const{
  return m_canSave;
}

const std::string& Format::GetDefaultExtension() const{
  return m_extensions.front();
}

const std::string& Format::GetLabel() const{
  return m_label;
}

bool Format::Match( const extension_t& ext ) const {
  return std::find(m_extensions.begin(), m_extensions.end(), lowercase(ext.Get())) != m_extensions.end();
}
