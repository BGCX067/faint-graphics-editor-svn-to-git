import os
faintFolders = [
    "",
    "app/",
    "bitmap/",
    "commands/",
    "formats/",
    "geo/",
    "gui/",
    "objects/",
    "python/",
    "python/generate/output",
    "rendering/",
    "tasks/",
    "tools/",
    "util/"
    ]

faintRoot = (os.path.abspath("../") + "/").replace("\\","/")

def enumerate_cpp(folder):
    return [file for file in os.listdir( folder ) if file.endswith('.cpp')]

def enumerate_all_sources(root):
    src = []
    for folder in faintFolders:
        for file in [file for file in os.listdir(os.path.join(root,folder)) if file.endswith('cpp') or file.endswith('.hh')]:
            src.append(os.path.join(root, folder, file))
    return src
