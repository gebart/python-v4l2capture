python-v4l2capture 1.4.x
Python extension to capture video with video4linux2

2009, 2010, 2011 Fredrik Portstrom
2011 Joakim Gebart

I, the copyright holder of this file, hereby release it into the
public domain. This applies worldwide. In case this is not legally
possible: I grant anyone the right to use this work for any purpose,
without any conditions, unless such conditions are required by law.

Introduction
============

python-v4l2capture is a slim and easy to use Python extension for
capturing video with video4linux2. It supports libv4l to convert any
image format to RGB or YUV420.

this fork of python-v4l2capture: https://github.com/gebart/python-v4l2capture

original python-v4l2capture: http://fredrik.jemla.eu/v4l2capture

libv4l: http://freshmeat.net/projects/libv4l

Installation
============

v4l2capture requires libv4l by default. You can compile v4l2capture
without libv4l, but that reduces image format support to YUYV input
and RGB output only. You can do so by commenting the line:

	libraries=["v4l2"], extra_compile_args=['-DUSE_LIBV4L', ],

in setup.py.

python-v4l2capture uses distutils. To build:

	./setup.py build

To build and install:

	./setup.py install

Example
=======

See capture_picture.py, capture_picture_delayed.py and list_devices.py.

The program filmroller.py provided a gui to take captures from a webcam. It
switches resolution between lowest resolution (for a live view) and highest
resolution (for the snapshot). It is quite useful to be used with slide and
negative film scanners.

![Sample picture of scanner](filmroller.device.jpg)

Change log
==========

(see git log for latest changes)

(2014-12-26) - Added framesize and frameinterval getters.

1.4 (2011-03-18) - Added support for YUV420 output.

1.3 (2010-07-21) - Added set of capabilities to the return value of
                   get_info. Updated list_devices.py.

1.2 (2010-04-01) - Forked example script into capture_picture.py and
    		   capture_picture_delayed.py.

1.1 (2009-11-03) - Updated URL and documentation.

1.0 (2009-02-28) - Initial release.
