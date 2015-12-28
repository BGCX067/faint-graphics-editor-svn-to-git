#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
from faint_scripts.log import Logger

assert(len(images) == 1)

log = Logger("run-test-color-background.py")

def test_file(name):
    return os.path.join(os.getcwd(), "out", name)

def create_file(path):
    with open(path,'w') as f:
        f.write("Generated by run-test-color-background.py")

create_file(test_file("file.color"))
create_file(test_file("file.bitmap"))

# Color background instead of Bitmap.
def color_background(file_path, image_props):
    frame = image_props.add_frame((10,10))
    frame.set_background(((255,0,255),(10,10)))

def test():
    add_format(color_background, None, "Color background", "color")
    app.open_files((test_file("file.color"),))

    log.error_if(len(images) != 2, "Failed opening.")

    # No bitmap allocated

    log.error_if(images[1].get_bitmap() is not None, "Bitmap was allocated.")
    log.error_if(images[1].test_has_bitmap(), "test_has_bitmap() True")

    images[1].auto_crop()
    log.error_if(images[1].get_size() != (10,10), "file.color - Wrong size")
    log.error_if(images[1].test_has_bitmap(), "Has bitmap.")
    images[1].rect(0,0,2,2)
    log.error_if(not images[1].test_has_bitmap(), "Bitmap not created "
                 "by drawing rectangle.")
    images[1].undo()
    log.error_if(images[1].test_has_bitmap(), "Did not revert to "
                 "color background on undo.")

    # Bitmap background
    def bitmap_background(file_path, image_props):
        frame = image_props.add_frame()
        frame.set_background(Bitmap((10,10),(255,0,0)))

    add_format(bitmap_background, None, "Bitmap background", "bitmap")
    app.open_files((test_file("file.bitmap"),))

    log.error_if(len(images) != 3, "Failed opening file.bitmap")

    log.error_if(not (images[2].get_bitmap().get_size() == (10,10)),
                 "file.bitmap - Wrong size")

    os.remove(test_file("file.color"))
    os.remove(test_file("file.bitmap"))

test()