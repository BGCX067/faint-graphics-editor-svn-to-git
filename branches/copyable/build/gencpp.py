# Copyright 2012 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import sys
if __name__ == '__main__': sys.path.append("../build-sys/");
import os

import build_sys.util.util as util

class Texts:
    pass

class Files:
    pass

class Defs:
    pass

def _need_generate(root, sources):
    outfiles = ["output/generated-methods.hh", "output/interface.cpp", "output/interface.hh", "output/py_settings_setget.hh"]
    newest = max([os.path.getmtime(f) for f in sources])

    for f in outfiles:
        if not os.path.isfile(f) or os.path.getmtime(f) < newest:
            return True
    return False

def non_shorthand_keys( stringmap ):
    # Retrieve the keys but ignore all shorthand forms
    # i.e., ignore keys that would translate a value like TRANSPARENT to a short form like "t"
    return [ key for key in stringmap.keys() if stringmap[key].__class__ == "".__class__ ]

def generate_int_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$min_value" : str(item.min_value),
             "$max_value" : str(item.max_value)}

    for line in Texts.template_int_cc:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetCC.write(line)
    for line in Texts.template_hh:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetHH.write( line )
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("set_" + item.py_name, "set_" + repl["$name"], item.doc_str))
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("get_" + item.py_name, "get_" + repl["$name"], item.doc_str))

def generate_float_function( setting, item ):
    repl = { "$name" : setting.lower(), # Function-setting-name, eh
             "$setting" : setting,
             "$min_value" : str(item.min_value),
             "$max_value" : str(item.max_value) }

    for line in Texts.template_float_cc:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetCC.write(line)
    for line in Texts.template_hh:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetHH.write( line )
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("set_" + item.py_name, "set_" + repl["$name"], item.doc_str))
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("get_" + item.py_name, "get_" + repl["$name"], item.doc_str))

def generate_stringtoint_function( setting, item ):
    repl = { "$name" : setting.lower(),
            "$setting" : setting }

    stringmap = item.py_to_cpp_map
    for line in Texts.template_stringtoint_cc:
       for key in repl.keys():
           line = line.replace( key, repl[key] )

       if line.find( "$string_to_int" ) != -1:
           # Convert to a block of code turning the parameter into an integer
           keys = list(stringmap.keys())

           key = keys[0]
           # First if
           line = line.replace('$string_to_int',
                               '%s value = %s::MIN;\n' % (item.cpp_enum.name, item.cpp_enum.name) )
           Files.targetCC.write(line)
           Files.targetCC.write('  if ( s_value == utf8_string("%s") ){\n    value = %s;\n  }\n' %(key, get_value( stringmap, key ) ) )
           # Each else if
           for key in keys[1:]:
               str = '  else if ( s_value == utf8_string("%s") ){\n    value = %s;\n  }\n' %(key, get_value(stringmap, key ) )
               Files.targetCC.write(str)

           # Defaulting else (error case)
           Files.targetCC.write('  else{\n')
           Files.targetCC.write('     PyErr_SetString( PyExc_ValueError, "Invalid string" );\n' )
           Files.targetCC.write('     return nullptr;\n')
           Files.targetCC.write('  }')

       else:
           # Copy all non-template:y lines
           Files.targetCC.write(line)

    for line in Texts.template_hh:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetHH.write(line)

    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("set_" + item.py_name, "set_" + repl["$name"], item.doc_str))

def generate_float_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$min_value" : str(item.min_value),
             "$max_value" : str(item.max_value),
             "$prettyname" : item.py_name }
    helpStr = item.doc_str

    for line in Texts.template_settings_float:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", nullptr}' %
      (item.py_name, repl["$name"], repl["$name"], helpStr) )

def generate_int_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$min_value" : str(item.min_value),
             "$max_value" : str(item.max_value),
             "$prettyname" : item.py_name,
             }
    helpStr = item.doc_str

    for line in Texts.template_settings_int:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", nullptr}' %
                               (item.py_name, repl["$name"], repl["$name"], helpStr) )

def generate_color_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$prettyname" : item.py_name}
    helpStr = item.doc_str
    for line in Texts.template_settings_color:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", nullptr}' %
                               (item.py_name, repl["$name"], repl["$name"], helpStr ) )

def get_value( map, key ):
    """Returns the value part from the dict, ignoring shorthand elements etc. Hack alert, eh? """
    if map[key].__class__ == "".__class__:
        return map[key]
    else:
        return map[key][0]

def generate_stringtoint_settings_function( setting, item ):
    repl = {"$name" : setting.lower(),
            "$setting" : setting,
            "$prettyname" : item.py_name}
    stringmap = item.py_to_cpp_map
    helpString = item.doc_str

    for line in Texts.template_settings_stringtoint:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        if line.find( "$string_to_int" ) != -1:
            keys = list(stringmap.keys())
            line = line.replace('$string_to_int',
                               '%s value = %s::MIN;\n' % (item.cpp_enum.name, item.cpp_enum.name) )

            Files.targetSettingsHH.write( line )
            Files.targetSettingsHH.write('  if ( s_value == utf8_string("%s") ){\n    value = %s;\n  }\n' % (keys[0], get_value(stringmap, keys[0]) ) )
            for key in keys[1:]:
                str = '  else if ( s_value == utf8_string("%s") ){\n    value = %s;\n  }\n' %(key, get_value(stringmap, key ) )
                Files.targetSettingsHH.write(str)

            Files.targetSettingsHH.write('  else{\n')
            Files.targetSettingsHH.write('     PyErr_SetString( PyExc_ValueError, "Invalid string" );\n' )
            Files.targetSettingsHH.write('     return setter_fail;\n')
            Files.targetSettingsHH.write('  }\n')

        elif line.find( "$int_to_string") != -1:
            keys = list(non_shorthand_keys( stringmap ))

            key = keys[0]
            line = line.replace( '$int_to_string',
                                 'if ( i_value  == to_int(%s) ){\n    s_value = "%s";  \n  }' %(stringmap[key], key ) )

            Files.targetSettingsHH.write( line )
            for key in keys[1:]:
                str = '  else if ( i_value == to_int(%s) ) {\n    s_value = "%s";\n  }\n' %(stringmap[key], key )
                Files.targetSettingsHH.write(str)

            Files.targetSettingsHH.write('  else{\n')
            Files.targetSettingsHH.write('     PyErr_SetString( PyExc_ValueError, "Unknown value" );\n' )
            Files.targetSettingsHH.write('     return nullptr;\n')
            Files.targetSettingsHH.write('  }\n')
        else:
            Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", nullptr}' %
                               (item.py_name, repl["$name"], repl["$name"], helpString) )

def generate_bool_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$prettyname" : item.py_name
             }
    helpStr = item.doc_str

    for line in Texts.template_settings_bool:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", nullptr}' %
                               (item.py_name, repl["$name"], repl["$name"], helpStr) )

def generate_string_settings_function( setting, item ):
    repl = {"$name" : setting.lower(),
            "$setting" : setting,
            "$prettyname" : item.py_name}
    for line in Texts.template_settings_string:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", nullptr}' %
                               (item.py_name, repl["$name"], repl["$name"], item.doc_str) )

def parseRange( str ):
    return str.split("-")

def parseStringToInt( str ):
    str = str.replace("(","")
    str = str.replace(")","")
    components = str.split(";")
    values = []
    for part in components:
        values.append( part.split("=") )

    return values

def write_cpp_setting_decl( out, setting ):
    out.write("extern const %s %s;\n" % (setting.cpp_type, setting.cpp_name) )

def write_cpp_setting_impl( out, setting ):
    out.write("const %s %s;\n" % (setting.cpp_type, setting.cpp_name) )

def write_cpp_int_to_enum_impl( out, setting ):
    """Writes a C++-function for converting an integer value to the
    enumerated constant for the setting
    to_<py_name>(int)."""
    enum = setting.cpp_enum
    out.write('%s to_%s(int v){\n' % (enum.name, setting.py_name))
    condition = "if"

    for entry in enum.entries:
        out.write('  %s ( v == to_int(%s::%s) ){\n' % (condition, enum.name, entry))
        out.write('    return %s::%s;\n' % (enum.name, entry))
        out.write('  }\n')
        condition = "else if"

    out.write('  assert(false);\n')
    out.write('  return %s::%s;\n' % (enum.name, enum.entries[0]))
    out.write('}\n')
    out.write('\n')

def inverse_py_to_cpp_map( map ):
    # Inverse the mapping and exclude shorthand entries
    return {v:k for k,v in [item for item in map.items() if item[1].__class__ == str]}

def write_cpp_value_to_key( out, map ):
    condition = "if"
    for key in sorted(map.keys()):
        out.write('    %s ( value == to_int(%s) ){\n' % (condition, key))
        out.write('       return "%s";\n' % (map[key]))
        out.write('    }\n')
        condition = "else if"

    out.write('    assert(false);\n')
    out.write('    return "";\n')

def write_cpp_value_string_impl( out, settings ):
    """Writes a C++-function for converting the value for an integer setting to its
    mnemonic if one is available"""
    out.write("utf8_string value_string(const IntSetting& setting, int value){\n");

    enum_settings = [s for s in settings if settings[s].get_type() == "stringtoint"]
    condition = "if"
    for setting_id in enum_settings:
        setting = settings[setting_id]
        out.write('  %s ( setting == %s ){\n' % (condition, setting.cpp_name))
        write_cpp_value_to_key(out, inverse_py_to_cpp_map(setting.py_to_cpp_map))
        out.write('  }\n')
        condition = "else if"
    out.write('  std::stringstream ss;\n')
    out.write('  ss << value;\n')
    out.write('  return utf8_string(ss.str());\n')
    out.write('}\n')

def write_cpp_to_int_impl( out, setting ):
    """Writes a C++-function for casting an enum value to an
    integer"""
    out.write("int to_int(%s v){\n" % setting.cpp_enum.name)
    out.write("  return static_cast<int>(v);\n")
    out.write("}\n")
    out.write("\n")

def write_cpp_valid_impl( out, setting ):
    """Writes a C++-function for checking if the integer value matches a
    constant for the setting"""
    enum = setting.cpp_enum
    out.write('bool valid_%s(int v){\n' % (setting.py_name))
    out.write('  return to_int(%s::MIN) <= v && v <= to_int(%s::MAX);\n' % (enum.name, enum.name))
    out.write('}\n')
    out.write('\n')

def write_cpp_stringtoint( out, setting ):
    enum = setting.cpp_enum
    entries = list(enum.entries)
    entries.append("MIN=%s" % enum.entries[0])
    entries.append("MAX=%s" % enum.entries[-1])
    entries.append("DEFAULT=%s" % enum.entries[0])
    out.write("enum class %s{\n" % enum.name)
    out.write("  " + ",\n  ".join(entries) + "\n")
    out.write("};\n")
    out.write("\n")
    write_cpp_setting_decl(out, setting)
    out.write("%s to_%s(int);\n" % (enum.name, setting.py_name) )
    out.write("bool valid_%s(int);\n" % (setting.py_name))
    out.write("int to_int(%s);\n\n" % (enum.name))

def write_cpp_header( file_name, settings ):
    out = open( file_name, 'w' )
    out.write('// Generated by %s\n' % os.path.basename(__file__))
    out.write('#ifndef %s\n' % util.create_include_guard(file_name))
    out.write('#define %s\n' % util.create_include_guard(file_name))
    out.write('#include "util/settings.hh"\n\n')
    out.write('\n')
    out.write('namespace faint{\n')
    for setting_id in sorted(settings.keys()):
        setting = settings[setting_id]
        if setting.get_type() == "stringtoint":
            write_cpp_stringtoint(out, setting)
        else:
            write_cpp_setting_decl(out, setting)
            out.write("\n")

    out.write("utf8_string setting_name(const UntypedSetting&);\n");
    out.write("utf8_string setting_name_pretty(const UntypedSetting&);\n");

    out.write("\n")
    out.write("// Returns the name for this value if the IntSetting has names for values\n")
    out.write("// otherwise, just returns the value as a string\n")
    out.write("utf8_string value_string(const IntSetting&, int value);\n");
    out.write("\n")
    out.write('} // namespace\n')
    out.write("\n")
    out.write("#endif\n")

def write_cpp_impl( file_name, include_file, settings ):
    out = open( file_name, 'w')
    out.write('// Generated by %s\n' % os.path.basename(__file__))
    out.write('#include <sstream>\n')
    out.write('#include "%s"\n' % include_file)
    out.write('\n')
    out.write('namespace faint{\n')
    for setting_id in sorted(settings.keys()):
        setting = settings[setting_id]
        write_cpp_setting_impl(out, setting)
    out.write("\n")

    for setting_id in sorted(settings.keys()):
        setting = settings[setting_id]
        if setting.get_type == "stringtoint":
            write_cpp_int_to_enum_impl(out, setting)

    write_setting_name_impl(out, settings, pretty=False)
    write_setting_name_impl(out, settings, pretty=True)

    for setting_id in sorted(settings.keys()):
        setting = settings[setting_id]
        if setting.get_type() == "stringtoint":
            write_cpp_int_to_enum_impl(out, setting)
            write_cpp_valid_impl(out, setting)
            write_cpp_to_int_impl(out, setting)

    write_cpp_value_string_impl(out, settings)

    out.write('} // namespace\n')

def write_setting_name_impl( out, settings, pretty ):
    if not pretty:
        out.write('utf8_string setting_name( const UntypedSetting& s ){\n')
    else:
        out.write('utf8_string setting_name_pretty( const UntypedSetting& s ){\n')

    condition = "if"
    for setting_id in sorted(settings.keys()):
        setting = settings[setting_id]
        out.write('  %s ( s == %s ){\n' % (condition, setting.cpp_name))
        out.write('    return "%s";\n' % ( setting.pretty_name if pretty else setting.py_name ))
        out.write('  }\n')
        condition = "else if"
    out.write('  assert(false);\n')
    out.write('  return "undefined_setting_name";\n')
    out.write('}\n')
    out.write('\n')

def run(root_dir, force=False):
    oldDir = os.getcwd()
    os.chdir(root_dir)
    sys.path.append(os.getcwd())
    if not force and not _need_generate("set_and_get.py", [os.path.join("templates", f) for f in os.listdir("templates")]):
        print(" Up to date.")
        os.chdir(oldDir)
        return
    print("Generating: interface.hh, interface.cpp, generated_methods.hh, py_settings_setget.hh")
    Files.targetCC = open("output/interface.cpp", 'w')
    Files.targetHH = open("output/interface.hh", 'w')
    Files.targetSettingsHH = open("output/py_settings_setget.hh", 'w')

    # Write the preamble
    for f in Files.targetCC, Files.targetHH, Files.targetSettingsHH:
        f.write('// Generated by %s\n' % os.path.basename(__file__))
    Files.targetHH.write('#ifndef %s\n' % util.create_include_guard('output/interface.hh'))
    Files.targetHH.write('#define %s\n' % util.create_include_guard('output/interface.hh'))
    Files.targetHH.write('\n')
    Files.targetHH.write('namespace faint{\n')

    Files.targetSettingsHH.write('#ifndef %s\n' % util.create_include_guard('output/py_settings_setget.hh'))
    Files.targetSettingsHH.write('#define %s\n' % util.create_include_guard('output/py_settings_setget.hh'))
    Files.targetSettingsHH.write("\n")
    Files.targetSettingsHH.write('namespace faint{\n')


    Files.targetCC.write('#include <string>\n');
    Files.targetCC.write('#include "python/py-include.hh"\n')
    Files.targetCC.write('#include "app/get-app-context.hh"\n');
    Files.targetCC.write('#include "util/settings.hh"\n')
    Files.targetCC.write('#include "util/setting-id.hh"\n')
    Files.targetCC.write('#include "util/canvas.hh"\n');
    Files.targetCC.write('\n\n')
    Files.targetCC.write('namespace faint{\n')

    # Collects all defined methods
    Defs.methodDef = []
    Defs.settings_methodDef = []

    # C++ code templates
    Texts.template_hh = open("templates/header_template.txt").readlines()
    Texts.template_int_cc = open("templates/int_template.txt").readlines()
    Texts.template_float_cc = open("templates/float_template.txt").readlines()
    Texts.template_stringtoint_cc = open("templates/stringtoint_template.txt").readlines()
    Texts.template_settings_header = open("templates/settings_header_template.txt").readlines()
    Texts.template_settings_float = open("templates/settings_float_template.txt").readlines();
    Texts.template_settings_int = open("templates/settings_int_template.txt").readlines();
    Texts.template_settings_color = open("templates/settings_color_template.txt").readlines()
    Texts.template_settings_stringtoint = open("templates/settings_stringtoint_template.txt").readlines()
    Texts.template_settings_bool = open("templates/settings_bool_template.txt").readlines()
    Texts.template_settings_string = open("templates/settings_string_template.txt").readlines()


    import set_and_get
    setters_and_getters = set_and_get.setters_and_getters

    for setting in setters_and_getters:
        item = setters_and_getters[setting]
        type = item.get_type()

        if type == "int":
            generate_int_function( setting, item )
            generate_int_settings_function( setting, item )

        elif type == "stringtoint":
            generate_stringtoint_function( setting, item )
            generate_stringtoint_settings_function( setting, item )

        elif type == "float":
            generate_float_function( setting, item )
            generate_float_settings_function( setting, item )

        elif type == "color":
            generate_color_settings_function( setting, item)
        elif type == "bool":
            generate_bool_settings_function( setting, item )
        elif type == "string":
            generate_string_settings_function( setting, item )

    # Create the python-export
    f_genMethods = open('output/generated-methods.hh', 'w')
    f_genMethods.write("#ifndef FAINT_GENERATED_METHODS_HH\n")
    f_genMethods.write("#define FAINT_GENERATED_METHODS_HH\n")
    f_genMethods.write('// Generated by %s\n' % os.path.basename(__file__))

    for index, line in enumerate(Defs.methodDef):
        f_genMethods.write( ' ' + line + ",\n" )
    f_genMethods.write("#endif")

    Files.targetSettingsHH.write("static PyGetSetDef settings_getseters[] = {\n");
    for line in Defs.settings_methodDef:
        Files.targetSettingsHH.write( line + ",\n" )
    Files.targetSettingsHH.write(" {0,0,0,0,0} // Sentinel */\n")
    Files.targetSettingsHH.write("};\n")

    Files.targetSettingsHH.write('} // namespace\n')

    write_cpp_header("output/cpp_settingid.hh", setters_and_getters)
    write_cpp_impl("output/cpp_settingid.cpp", "cpp_settingid.hh", setters_and_getters)

    Files.targetHH.write("\n")
    Files.targetHH.write('} // namespace\n')
    Files.targetHH.write("#endif\n")

    Files.targetCC.write('} // namespace\n')

    # Close generated files
    Files.targetCC.close()
    Files.targetHH.close()

    Files.targetSettingsHH.write("\n")
    Files.targetSettingsHH.write("#endif")
    Files.targetSettingsHH.close()

    os.chdir(oldDir)

if __name__ == '__main__':
    force = "--force" in sys.argv
    run("../python/generate", force)
