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
    name = "v4l2capture",
    version = "1.4",
    author = "Fredrik Portstrom, Tim Sheerman-Chase",
    author_email = "fredrik@jemla.se",
    url = "http://fredrik.jemla.eu/v4l2capture",
    description = "Capture video with video4linux2",
    long_description = "python-v4l2capture is a slim and easy to use Python "
    "extension for capturing video with video4linux2.",
    license = "Public Domain",
    classifiers = [
        "License :: Public Domain",
        "Programming Language :: C++"],
    ext_modules = [
        Extension("v4l2capture", ["v4l2capture.cpp"], libraries = ["v4l2", "pthread", "jpeg"])])

