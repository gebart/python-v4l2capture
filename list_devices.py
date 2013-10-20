#!/usr/bin/python
#
# python-v4l2capture
#
# python-v4l2capture
# Python extension to capture video with video4linux2
#
# 2009, 2010, 2011 Fredrik Portstrom, released into the public domain
# 2011, Joakim Gebart
# 2013, Tim Sheerman-Chase
# See README for license

import os
import v4l2capture
file_names = [x for x in os.listdir("/dev") if x.startswith("video")]
file_names.sort()
for file_name in file_names:
    path = "/dev/" + file_name
    print path
    try:
        video = v4l2capture.Video_device(path)
        driver, card, bus_info, capabilities = video.get_info()
        print "    driver:       %s\n    card:         %s" \
            "\n    bus info:     %s\n    capabilities: %s" % (
                driver, card, bus_info, ", ".join(capabilities))
        video.close()
    except IOError, e:
        print "    " + str(e)
