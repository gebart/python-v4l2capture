#!/usr/bin/env python3
#
# python-v4l2capture
#
# 2009, 2010 Fredrik Portstrom
#
# I, the copyright holder of this file, hereby release it into the
# public domain. This applies worldwide. In case this is not legally
# possible: I grant anyone the right to use this work for any
# purpose, without any conditions, unless such conditions are
# required by law.

import os
import v4l2capture


def get_devices():
    file_names = [x for x in os.listdir("/dev") if x.startswith("video")]
    file_names.sort()
    devices = {}
    for file_name in file_names:
        path = "/dev/" + file_name
        print(path)
        try:
            video = v4l2capture.Video_device(path)
            print(video)
            driver, card, bus_info, capabilities = video.get_info()
            devices[path] = {}
            devices[path]['driver'] = driver
            devices[path]['card'] = card
            devices[path]['bus_info'] = bus_info
            devices[path]['capabilities'] = capabilities
            video.close()
        except IOError as e:
            print(e)
    return devices


if __name__ == '__main__':
    print(get_devices())
