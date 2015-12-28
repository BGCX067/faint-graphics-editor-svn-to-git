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
import os

class Texts:
    pass

class Files:
    pass

class Defs:
    pass

def _need_generate(root, sources):
    outfiles = ["output/generated_methods.hh", "output/interface.cpp", "output/interface.hh", "output/py_settings_setget.hh"]
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
             "$min_value" : str(item[1][0]),
             "$max_value" : str(item[1][1])
             }

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
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("set_" + item[2], "set_" + repl["$name"], ""))
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("get_" + item[2], "get_" + repl["$name"], ""))

def generate_float_function( setting, item ):
    repl = { "$name" : setting.lower(), # Function-setting-name, eh
             "$setting" : setting,
             "$min_value" : str(item[1][0]),
             "$max_value" : str(item[1][1]),
             }

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
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("set_" + item[2], "set_" + repl["$name"], ""))
    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("get_" + item[2], "get_" + repl["$name"], ""))

def generate_stringtoint_function( setting, item ):
    repl = { "$name" : setting.lower(),
            "$setting" : setting }

    stringmap = item[1]
    for line in Texts.template_stringtoint_cc:
       for key in repl.keys():
           line = line.replace( key, repl[key] )

       if line.find( "$string_to_int" ) != -1:
           # Convert to a block of code turning the parameter into an integer
           keys = stringmap.keys()

           key = keys[0]
           # First if
           line = line.replace( '$string_to_int',
                                'if ( strcmp(s_value, "%s" ) == 0 ){\n    i_value = %s;  \n  }' %(key, get_value( stringmap, key ) ) )
           Files.targetCC.write(line)
           # Each else if
           for key in keys[1:]:
               str = '  else if ( strcmp(s_value, "%s" ) == 0 ){\n    i_value = %s;\n  }\n' %(key, get_value(stringmap, key ) )
               Files.targetCC.write(str)

           # Defaulting else (error case)
           Files.targetCC.write('  else{\n')
           Files.targetCC.write('     PyErr_SetString( PyExc_ValueError, "Invalid string" );\n' )
           Files.targetCC.write('     return 0;\n')
           Files.targetCC.write('  }')

       else:
           # Copy all non-template:y lines
           Files.targetCC.write(line)

    for line in Texts.template_hh:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetHH.write(line)

    Defs.methodDef.append( '{"%s", %s, METH_VARARGS, "%s"}' % ("set_" + item[2], "set_" + repl["$name"], ""))

def generate_float_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$min_value" : str(item[1][0]),
             "$max_value" : str(item[1][1]),
             "$prettyname" : item[2]
             }
    helpStr = item[3]

    for line in Texts.template_settings_float:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", NULL}' %
                               (item[2], repl["$name"], repl["$name"], helpStr) )

def generate_int_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$min_value" : str(item[1][0]),
             "$max_value" : str(item[1][1]),
             "$prettyname" : item[2],
             }
    helpStr = item[3]

    for line in Texts.template_settings_int:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", NULL}' %
                               (item[2], repl["$name"], repl["$name"], helpStr) )

def generate_color_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$prettyname" : item[1]}
    helpStr = item[2]
    for line in Texts.template_settings_color:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", NULL}' %
                               (item[1], repl["$name"], repl["$name"], helpStr ) )

def get_value( map, key ):
    """Returns the value part from the dict, ignoring shorthand elements etc. Hack alert, eh? """
    if map[key].__class__ == "".__class__:
        return map[key]
    else:
        return map[key][0]

def generate_stringtoint_settings_function( setting, item ):
    repl = {"$name" : setting.lower(),
            "$setting" : setting,
            "$prettyname" : item[2]}
    stringmap = item[1]
    helpString = item[3]

    for line in Texts.template_settings_stringtoint:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        if line.find( "$string_to_int" ) != -1:
            keys = stringmap.keys()
            line = line.replace( '$string_to_int',
                                 'if ( strcmp(s_value, "%s" ) == 0 ){\n    i_value = %s;  \n  }' % (keys[0], get_value(stringmap, keys[0]) ) )

            Files.targetSettingsHH.write( line )

            for key in keys[1:]:
                str = '  else if ( strcmp(s_value, "%s" ) == 0 ){\n    i_value = %s;\n  }\n' %(key, get_value(stringmap, key ) )
                Files.targetSettingsHH.write(str)

            Files.targetSettingsHH.write('  else{\n')
            Files.targetSettingsHH.write('     PyErr_SetString( PyExc_ValueError, "Invalid string" );\n' )
            Files.targetSettingsHH.write('     return -1;\n')
            Files.targetSettingsHH.write('  }\n')

        elif line.find( "$int_to_string") != -1:
            keys = non_shorthand_keys( stringmap )

            key = keys[0]
            line = line.replace( '$int_to_string',
                                 'if ( i_value  == %s ){\n    s_value = "%s";  \n  }' %(stringmap[key], key ) )

            Files.targetSettingsHH.write( line )
            for key in keys[1:]:
                str = '  else if ( i_value == %s ) {\n    s_value = "%s";\n  }\n' %(stringmap[key], key )
                Files.targetSettingsHH.write(str)

            Files.targetSettingsHH.write('  else{\n')
            Files.targetSettingsHH.write('     PyErr_SetString( PyExc_ValueError, "Invalid string" );\n' )
            Files.targetSettingsHH.write('     return 0;\n')
            Files.targetSettingsHH.write('  }\n')
        else:
            Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", NULL}' %
                               (item[2], repl["$name"], repl["$name"], helpString) )

def generate_bool_settings_function( setting, item ):
    repl = { "$name" : setting.lower(),
             "$setting" : setting,
             "$prettyname" : item[1]
             }
    helpStr = item[2]

    for line in Texts.template_settings_bool:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", NULL}' %
                               (item[1], repl["$name"], repl["$name"], helpStr) )

def generate_string_settings_function( setting, item ):
    repl = {"$name" : setting.lower(),
            "$setting" : setting,
            "$prettyname" : item[1]}
    helpStr = item[2]
    for line in Texts.template_settings_string:
        for key in repl.keys():
            line = line.replace( key, repl[key] )
        assert line.find("$") == -1
        Files.targetSettingsHH.write(line)
    Defs.settings_methodDef.append( '   {(char*)"%s",\n   GETTER(settings_get_%s),\n   SETTER(settings_set_%s), \n   (char*)"%s", NULL}' %
                               (item[1], repl["$name"], repl["$name"], helpStr) )

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

def run():
    sys.path.append(os.getcwd())
    if not _need_generate("set_and_get.py", [os.path.join("templates", f) for f in os.listdir("templates")]):
        print " Up to date."
        return
    print "Generating: interface.hh, interface.cpp, generated_methods.hh, py_settings_setget.hh"
    Files.targetCC = open("output/interface.cpp", 'w')
    Files.targetHH = open("output/interface.hh", 'w')
    Files.targetSettingsHH = open("output/py_settings_setget.hh", 'w')

    # Write the preamble
    for f in Files.targetCC, Files.targetHH, Files.targetSettingsHH:
        f.write("// Generated by gen_settinterface.py\n")

    Files.targetCC.write('#include "python/pythoninclude.hh"\n')
    Files.targetCC.write('#include <string>\n');
    Files.targetCC.write('#include "util/settings.hh"\n')
    Files.targetCC.write('#include "util/settingid.hh"\n')
    Files.targetCC.write('#include "app/getappcontext.hh"\n');
    Files.targetCC.write('#include "util/canvasinterface.hh"\n');
    Files.targetCC.write('\n\n')

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


    from set_and_get import setters_and_getters
    for setting in setters_and_getters:
        item = setters_and_getters[setting]
        type = item[0]

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
    f_genMethods = open('output/generated_methods.hh', 'w')
    f_genMethods.write('#include "interface.hh"\n')
    f_genMethods.write('static PyMethodDef generated_methods[] = {\n')
    for index, line in enumerate(Defs.methodDef):
        f_genMethods.write( ' ' + line + ",\n" )

    f_genMethods.write(" {NULL, NULL, 0, NULL}\n")
    f_genMethods.write('};\n')

    Files.targetSettingsHH.write("// (char*)-casts silence a gcc warning about const char* to char*\n")
    Files.targetSettingsHH.write("// ...I hope this is safe\n")
    Files.targetSettingsHH.write("static PyGetSetDef settings_getseters[] = {\n");
    for line in Defs.settings_methodDef:
        Files.targetSettingsHH.write( line + ",\n" )
    Files.targetSettingsHH.write(" {0,0,0,0,0} // Sentinel */\n")
    Files.targetSettingsHH.write("};")

    # Close generated files
    Files.targetCC.close()
    Files.targetHH.close()
    Files.targetSettingsHH.close()

if __name__ == '__main__':
    run()
