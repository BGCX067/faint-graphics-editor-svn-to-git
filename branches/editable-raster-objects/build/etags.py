import ConfigParser
import os
import subprocess
import faint_info

if not os.path.exists( "build.cfg" ):
    print "Error: build.cfg missing. Run build.py first."
    exit(1)

# Get the path to the etags application
config = ConfigParser.RawConfigParser()
config.read('build.cfg')
etags_folder = config.get("other", "etags_folder")
etags_exe = os.path.join(etags_folder, "etags.exe")

# List all Faint source files
sourceFiles = ['"%s"' % file for file in faint_info.enumerate_all_sources(faint_info.faintRoot)]

etags = subprocess.Popen('"%s" %s' % (etags_exe, " ".join(sourceFiles)))
if etags.wait() != 0:
    print "Error generating TAGS file."
    exit(1)



