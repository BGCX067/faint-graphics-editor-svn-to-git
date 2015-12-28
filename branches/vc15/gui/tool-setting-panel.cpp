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

#include "wx/gbsizer.h"
#include "wx/statline.h"
#include "gui/art-container.hh"
#include "gui/font-ctrl.hh"
#include "gui/freezer.hh"
#include "gui/image-toggle-ctrl.hh"
#include "gui/spin-ctrl.hh"
#include "gui/tool-setting-ctrl.hh"
#include "gui/tool-setting-panel.hh"
#include "util/setting-id.hh"
#include "util/settings.hh"

namespace faint{

static void add_control(ToolSettingCtrl* ctrl, wxSizer* sizer, std::list<ToolSettingCtrl*>& controlList){
  sizer->Add(ctrl, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5);
  controlList.push_back(ctrl);
}

ToolSettingPanel::ToolSettingPanel(wxWindow* parent,
  StatusInterface& status,
  ArtContainer& art,
  DialogContext& dialogContext) :
  wxPanel(parent)
{
  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  // Initial separator
  sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition), 0, wxEXPAND|wxUP|wxDOWN, 5);

  add_control(new_bool_image_toggle(this, ts_EditPoints, art.Get(Icon::EDIT_POINTS), status, tooltip_t("Edit Points")),
    sizer, m_toolControls);

  add_control(new_semi_float_spinner(this, wxSize(50, -1), ts_LineWidth, 1, "Line width"),
    sizer, m_toolControls);

  add_control(new_int_spinner(this, wxSize(50, -1), ts_BrushSize, 5, "Brush Size"),
    sizer, m_toolControls);

  add_control(new_font_ctrl(this, dialogContext),
    sizer, m_toolControls);

  add_control(new_int_spinner(this, wxSize(50, -1), ts_FontSize, 12, "Size"),
    sizer, m_toolControls);

  add_control(new_bool_image_toggle(this, ts_BoundedText, art.Get(Icon::TEXT_BOUNDED), status, tooltip_t("Bounded by rectangle")),
    sizer, m_toolControls);

  add_control(new_bool_image_toggle(this, ts_ParseExpressions,
      art.Get(Icon::TEXT_PARSING), status, tooltip_t("Parse special commands")),
    sizer, m_toolControls);

  add_control(new_bool_image_toggle(this, ts_ClosedPath, art.Get(Icon::PATH_CLOSED), status, tooltip_t("Closed path")),
    sizer, m_toolControls);

  const IntSize iconSize(28, 23);

  add_control(new_image_toggle(this, ts_HorizontalAlign,
      iconSize,
      status,
      tooltip_t("Text alignment"),
      ToggleImage(art.Get(Icon::TEXT_ALIGN_LEFT),
        to_int(HorizontalAlign::LEFT), "Left aligned text"),
      ToggleImage(art.Get(Icon::TEXT_ALIGN_CENTER),
        to_int(HorizontalAlign::CENTER), "Centered text"),
      ToggleImage(art.Get(Icon::TEXT_ALIGN_RIGHT),
        to_int(HorizontalAlign::RIGHT), "Right aligned text")),
    sizer,
    m_toolControls);

  add_control(new_image_toggle(this,
      ts_LineStyle,
      IntSize(28,15),
      status,
      tooltip_t("Line style"),
      ToggleImage(art.Get(Icon::LINESTYLE_SOLID),
        to_int(LineStyle::SOLID), "Solid Lines"),
      ToggleImage(art.Get(Icon::LINESTYLE_LONG_DASH),
        to_int(LineStyle::LONG_DASH), "Dashed lines")),
    sizer,
    m_toolControls);

  add_control(new_image_toggle(this,
      ts_PointType,
      iconSize,
      status,
      tooltip_t("Segment type"),
      ToggleImage(art.Get(Icon::POINTTYPE_LINE),
        to_int(PointType::LINE), "Line"),
      ToggleImage(art.Get(Icon::POINTTYPE_CURVE),
        to_int(PointType::CURVE), "Curve")),
    sizer,
    m_toolControls);

  add_control(new_image_toggle(this,
      ts_FillStyle, iconSize,
      status,
      tooltip_t("Fill style"),
      ToggleImage(art.Get(Icon::FILLSTYLE_BORDER),
        to_int(FillStyle::BORDER), "Border Only"),
      ToggleImage(art.Get(Icon::FILLSTYLE_FILL),
        to_int(FillStyle::FILL), "Fill Only"),
      ToggleImage(art.Get(Icon::FILLSTYLE_BORDER_AND_FILL),
        to_int(FillStyle::BORDER_AND_FILL), "Border and Fill")),
    sizer, m_toolControls);

  add_control(new_image_toggle(this,
      ts_BrushShape,
      iconSize,
      status,
      tooltip_t("Brush shape"),
      ToggleImage(art.Get(Icon::BRUSH_CIRCLE),
        to_int(BrushShape::CIRCLE), "Circular Brush"),
      ToggleImage(art.Get(Icon::BRUSH_RECT),
        to_int(BrushShape::SQUARE), "Square Brush"),
      ToggleImage(art.Get(Icon::BRUSH_EXPERIMENTAL),
        to_int(BrushShape::EXPERIMENTAL), "Experimental Brush")),
    sizer,
    m_toolControls);

  add_control(new_image_toggle(this,
      ts_LineArrowhead,
      iconSize,
      status,
      tooltip_t("Line end style"),
      ToggleImage(art.Get(Icon::LINE_NO_ARROW), 0, "No arrowhead"),
      ToggleImage(art.Get(Icon::LINE_ARROW_FRONT), 1, "Forward arrowhead")),
    sizer,
    m_toolControls);

  add_control(new_image_toggle(this,
      ts_BackgroundStyle,
      iconSize,
      status,
      tooltip_t("Transparent or opaque background color"),
      ToggleImage(art.Get(Icon::CHOICE_TRANSPARENT),
        to_int(BackgroundStyle::MASKED), "Transparent Background"),
      ToggleImage(art.Get(Icon::CHOICE_OPAQUE),
        to_int(BackgroundStyle::SOLID), "Opaque Background")),
    sizer,
    m_toolControls);

  add_control(new_bool_image_toggle(this,
      ts_PolyLine,
      art.Get(Icon::LINE_POLYLINE),
      status,
      tooltip_t("Polyline")),
    sizer, m_toolControls);

  add_control(new_bool_image_toggle(this, ts_AlphaBlending, art.Get(Icon::ALPHA_BLENDING), status, tooltip_t("Alpha blending")),
    sizer, m_toolControls);

  // Finishing separator
  sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition), 0, wxEXPAND|wxUP|wxDOWN, 5);
  SetSizerAndFit(sizer);
}

void ToolSettingPanel::ShowSettings(const Settings& settings){
  auto freezer = freeze(this);
  for (auto ctrl : m_toolControls){
    bool show = ctrl->UpdateControl(settings);
    ctrl->Show(show);
  }
  Layout();
}

} // namespace
