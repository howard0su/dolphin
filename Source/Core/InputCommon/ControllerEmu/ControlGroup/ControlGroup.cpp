// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"

#include "Common/CommonTypes.h"
#include "Common/IniFile.h"

#include "InputCommon/ControlReference/ControlReference.h"
#include "InputCommon/ControllerEmu/Control/Input.h"
#include "InputCommon/ControllerEmu/Control/Output.h"
#include "InputCommon/ControllerEmu/ControlGroup/Attachments.h"
#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerEmu/Setting/NumericSetting.h"

namespace ControllerEmu
{
ControlGroup::ControlGroup(std::string name, const GroupType type, CanBeDisabled can_be_disabled_)
    : m_name(name), m_ui_name(std::move(name)), m_type(type),
      can_be_disabled(can_be_disabled_ == CanBeDisabled::Yes)
{
}

ControlGroup::ControlGroup(std::string name, std::string ui_name, const GroupType type,
                           CanBeDisabled can_be_disabled_)
    : m_name(std::move(name)), m_ui_name(std::move(ui_name)), m_type(type),
      can_be_disabled(can_be_disabled_ == CanBeDisabled::Yes)
{
}

void ControlGroup::AddDeadzoneSetting(SettingValue<double>* value, double maximum_deadzone)
{
  AddSetting(value,
             {_trans("Dead Zone"),
              // i18n: The percent symbol.
              _trans("%"),
              // i18n: Refers to the dead-zone setting of gamepad inputs.
              _trans("Input strength to ignore.")},
             0, 0, maximum_deadzone);
}

ControlGroup::~ControlGroup() = default;

void ControlGroup::LoadConfig(IniFile::Section* sec, const std::string& defdev,
                              const std::string& base)
{
  const std::string group(base + m_name + "/");

  // enabled
  if (can_be_disabled)
    sec->Get(group + "Enabled", &enabled, true);

  for (auto& setting : numeric_settings)
    setting->LoadFromIni(*sec, group);

  for (auto& c : controls)
  {
    {
      // control expression
      std::string expression;
      sec->Get(group + c->name, &expression, "");
      c->control_ref->SetExpression(std::move(expression));
    }

    // range
    sec->Get(group + c->name + "/Range", &c->control_ref->range, 100.0);
    c->control_ref->range /= 100;
  }

  // extensions
  if (m_type == GroupType::Attachments)
  {
    auto* const ext = static_cast<Attachments*>(this);

    ext->SetSelectedAttachment(0);
    u32 n = 0;
    std::string attachment_text;
    sec->Get(base + m_name, &attachment_text, "");

    // First assume attachment string is a valid expression.
    // If it instead matches one of the names of our attachments it is overridden below.
    ext->GetSelectionSetting().GetInputReference().SetExpression(attachment_text);

    for (auto& ai : ext->GetAttachmentList())
    {
      ai->SetDefaultDevice(defdev);
      ai->LoadConfig(sec, base + ai->GetName() + "/");

      if (ai->GetName() == attachment_text)
        ext->SetSelectedAttachment(n);

      n++;
    }
  }
}

void ControlGroup::SaveConfig(IniFile::Section* sec, const std::string& defdev,
                              const std::string& base)
{
  const std::string group(base + m_name + "/");

  // enabled
  sec->Set(group + "Enabled", enabled, true);

  for (auto& setting : numeric_settings)
    setting->SaveToIni(*sec, group);

  for (auto& c : controls)
  {
    // control expression
    sec->Set(group + c->name, c->control_ref->GetExpression(), "");

    // range
    sec->Set(group + c->name + "/Range", c->control_ref->range * 100.0, 100.0);
  }

  // extensions
  if (m_type == GroupType::Attachments)
  {
    auto* const ext = static_cast<Attachments*>(this);

    if (ext->GetSelectionSetting().IsSimpleValue())
    {
      sec->Set(base + m_name, ext->GetAttachmentList()[ext->GetSelectedAttachment()]->GetName(),
               "None");
    }
    else
    {
      sec->Set(base + m_name, ext->GetSelectionSetting().GetInputReference().GetExpression(), "None");
    }

    for (auto& ai : ext->GetAttachmentList())
      ai->SaveConfig(sec, base + ai->GetName() + "/");
  }
}

void ControlGroup::SetControlExpression(int index, const std::string& expression)
{
  controls.at(index)->control_ref->SetExpression(expression);
}

void ControlGroup::AddInput(Translatability translate, std::string name_)
{
  controls.emplace_back(std::make_unique<Input>(translate, std::move(name_)));
}

void ControlGroup::AddInput(Translatability translate, std::string name_, std::string ui_name_)
{
  controls.emplace_back(std::make_unique<Input>(translate, std::move(name_), std::move(ui_name_)));
}

void ControlGroup::AddOutput(Translatability translate, std::string name_)
{
  controls.emplace_back(std::make_unique<Output>(translate, std::move(name_)));
}

}  // namespace ControllerEmu
