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

#include <sstream>
#include "bitmap/color.hh"
#include "text/formatting.hh"
#include "util/setting-id.hh"
#include "util/settings.hh"
#include "python/mapped-type.hh"
#include "python/py-include.hh"
#include "python/py-settings.hh"
#include "python/py-util.hh"
#include "python/py-ugly-forward.hh"
#include "generated/python/settings/py-settings-properties.hh"

namespace faint{

template<>
struct MappedType<Settings&>{
  using PYTHON_TYPE = settingsObject;

  static Settings& GetCppObject(settingsObject* self){
    return *self->settings;
  }

  static bool Expired(settingsObject*){
    // The settingsObject owns the settings, it is not a reference
    // that can break.
    return false;
  }

  static void ShowError(settingsObject*){
    // See Expired
  }
};

static std::ostream& operator<<(std::ostream& o, const Paint& paint){
  o << dispatch(paint,
    [](const Color& c){return bracketed(str_rgba(c));},
    [](const Pattern&){return "Pattern";},
    [](const Gradient&){return "Gradient";});
  return o;
}

/* method: "update(settings)\n
Update the current settings with the values from the passed in
Settings." */
static void settings_update(Settings& self, const Settings& other){
  self.Update(other);
}

/* method: "update_all(settings)\n
Copy all the settings in the passed in Settings to self." */
static void settings_update_all(Settings& self, const Settings& other){
  self.UpdateAll(other);
}

#include "generated/python/method-def/py-settings-methoddef.hh"

// Python standard methods for Settings
static PyObject* settings_new(PyTypeObject* type, PyObject* , PyObject*){
  settingsObject* self = (settingsObject*)type->tp_alloc(type, 0);
  self->settings = nullptr;
  return (PyObject*)self;
}

static void settings_dealloc(settingsObject* self){
  delete self->settings;
  self->settings = nullptr;
  self->ob_base.ob_type->tp_free((PyObject*)self);
}

static PyObject* settings_str(settingsObject* self){
  const Settings& settings = *(self->settings);
  std::vector<UntypedSetting> raw = self->settings->GetRaw();
  std::stringstream ss;

  for (size_t i = 0; i != raw.size(); i++){
    ss << setting_name(raw[i]) << ": ";

    // IntSettings can be represented by strings in the Python interface,
    // if this is the case, show the string representation rather than the value
    IntSetting as_int(raw[i].ToInt());
    if (settings.Has(as_int)){
      ss << value_string(as_int, settings.Get(as_int));
    }
    else if (settings.Has(FloatSetting(raw[i].ToInt()))) {
      ss << settings.Get(FloatSetting(raw[i].ToInt()));
    }
    else if (settings.Has(BoolSetting(raw[i].ToInt()))) {
      ss << settings.Get(BoolSetting(raw[i].ToInt()));
    }
    else if (settings.Has(StrSetting(raw[i].ToInt()))) {
      ss << settings.Get(StrSetting(raw[i].ToInt()));
    }
    else if (settings.Has(ColorSetting(raw[i].ToInt()))) {
      ss << settings.Get(ColorSetting(raw[i].ToInt()));
    }
    if (i != raw.size() - 1){
      ss << ", ";
    }
  }
  return Py_BuildValue("s", ss.str().c_str());
}

static void settings_init(settingsObject& self){
  assert(self.settings == nullptr);
  self.settings = new Settings;
}

PyTypeObject SettingsType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "Settings", // tp_name
  sizeof(settingsObject), // tp_basicsize
  0, // tp_itemsize
  (destructor)settings_dealloc, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  0, // tp_repr
  0, // tp_as_number
  0, // tp_as_sequence
  0, // tp_as_mapping
  0, // tp_hash
  0, // tp_call
  (reprfunc)settings_str, // tp_str
  0, // tp_getattro
  0, // tp_setattro
  0, // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
  "A collection of Faint tool and object settings", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  settings_methods, // tp_methods
  0, // tp_members
  settings_properties, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  INIT_FORWARDER(settings_init), // tp_init
  0, // tp_alloc
  settings_new, // tp_new
  0, // tp_free
  0, // tp_is_gc
  0, // tp_bases
  0, // tp_mro
  0, // tp_cache
  0, // tp_subclasses
  0, // tp_weaklist
  0, // tp_del
  0, // tp_version_tag
  0  // tp_finalize
};

settingsObject* py_args_to_settings(PyObject* args){
  PyObject* o = nullptr;
  if (PyArg_ParseTuple(args, "O", &o)){
    if (PyObject_IsInstance(o, (PyObject*)&SettingsType)){
      return (settingsObject*)o;
    }
  }
  return nullptr;
}

PyObject* pythoned(const Settings& s){
  settingsObject* py_settings = (settingsObject*)SettingsType.tp_alloc(&SettingsType, 0);
  py_settings->settings = new Settings(s);
  return (PyObject*)py_settings;
}

} // namespace
