// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#include "util/pos-info.hh"

namespace faint{

void ToolModifiers::SetLeftMouse(){
  m_mouseButton = faint::MouseButton::LEFT;
}

void ToolModifiers::SetRightMouse(){
  m_mouseButton = faint::MouseButton::RIGHT;
}

void ToolModifiers::SetPrimary(){
  m_primary = true;
}

void ToolModifiers::SetSecondary(){
  m_secondary = true;
}

bool ToolModifiers::LeftMouse() const{
  return m_mouseButton == faint::MouseButton::LEFT;
}

bool ToolModifiers::RightMouse() const{
  return m_mouseButton == faint::MouseButton::RIGHT;
}

faint::MouseButton ToolModifiers::MouseButton() const{
  return m_mouseButton;
}

bool ToolModifiers::Feature(faint::MouseButton button) const{
  return m_mouseButton == button;
}

bool ToolModifiers::Primary() const{
  return m_primary;
}

bool ToolModifiers::Secondary() const{
  return m_secondary;
}

bool ToolModifiers::OnlyPrimary() const{
  return m_primary && !m_secondary;
}

bool ToolModifiers::OnlySecondary() const{
  return !m_primary && m_secondary;
}

bool ToolModifiers::Both() const{
  return m_primary && m_secondary;
}

bool ToolModifiers::Either() const{
  return m_primary || m_secondary;
}

bool ToolModifiers::Neither() const{
  return !(m_primary || m_secondary);
}

MouseButton the_other_one(MouseButton button){
  return button == MouseButton::LEFT?
    MouseButton::RIGHT :
    MouseButton::LEFT;
}

} // namespace
