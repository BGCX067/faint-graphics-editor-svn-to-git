import os
import sys

def clean_generated(faintDir):
    outDir = os.path.join(faintDir, "python", "generate", "output")
    for file in os.listdir(outDir):
        if file.endswith(".hh") or file.endswith(".cpp"):
            os.remove(os.path.join(outDir, file))

    generated_resources = os.path.join(faintDir, "generated")
    if os.path.exists(generated_resources):
        for file_name in os.listdir(generated_resources):
            if file_name == "index.txt" or file.endswith(".hh") or file.endswith(".cpp"):
                os.remove(os.path.join(generated_resources, file_name))
        os.rmdir(generated_resources)

    generated_makefile = os.path.join(faintDir, "build/generated_makefile")
    if os.path.exists(generated_makefile):
        os.remove(generated_makefile)

def clean_help(faintDir):
    helpDir = os.path.join(faintDir, "help")
    for file in os.listdir(helpDir):
        if file.endswith(".html") or file.endswith(".dat"):
            os.remove(os.path.join(helpDir, file))

def clean_obj(faintDir):
    for buildDir in (os.path.join(faintDir, "build", "objs"), os.path.join(faintDir, "build", "objs_debug")):
        if os.path.exists(buildDir):
            for file in os.listdir(buildDir):
                if file.endswith('.o') or file.endswith('.obj') or file.endswith(".res") or file.endswith(".pdb"):
                    os.remove(os.path.join(buildDir, file))

def clean_exe(faintDir):
    for file in os.listdir(faintDir):
        if file.endswith(".exe"):
            os.remove(os.path.join(faintDir, file))


known_args=(("all", "everything", (clean_help, clean_generated, clean_obj, clean_exe)),
            ("exe", "Faint executables", (clean_exe,)),
            ("html", "the help", (clean_help,)),
            ("gen", "generated code", (clean_generated,)),
            ("obj", "built object files", (clean_obj,)))

if __name__ == '__main__':
    if len(sys.argv) == 1:
        print "Arguments:"
        print " " + "\n ".join([arg[0] + "\t" + arg[1] for arg in known_args])
        exit(1)

    argnames = [ka[0] for ka in known_args]
    for arg in sys.argv[1:]:
        if arg not in argnames:
            print "Unknown argument: " + arg
            exit(1)

    rootDir = os.path.abspath("../")
    path_check = ("app", "build", "tools", "objects", "util")
    rootFiles = os.listdir(rootDir)
    for file in path_check:
        if file not in rootFiles:
            print "Error: Didn't find expected folders in %s" % rootDir
            exit(1)

    for arg in sys.argv:
        for ka in known_args:
            if arg == ka[0]:
                for func in ka[2]:
                    func(rootDir)
