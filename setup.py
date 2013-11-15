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

from distutils.core import Extension, setup
setup(
    name = "videolive",
    version = "1.0",
    author = "Tim Sheerman-Chase",
    author_email = "info@kinatomic",
    url = "http://www.kinatomic.com",
    description = "Capture and stream video",
    long_description = "Capture and stream video in python",
    license = "GPL v2 or later",
    classifiers = [
        "License :: GPL",
        "Programming Language :: C++"],
    ext_modules = [
        Extension("videolive", ["v4l2capture.cpp", "v4l2out.cpp", "pixfmt.cpp", "libvideolive.cpp", "videoout.cpp", "videoin.cpp"], 
			libraries = ["v4l2", "pthread", "jpeg"])])

