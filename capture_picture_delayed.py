#!/usr/bin/python
#
# python-v4l2capture
#
# This file is an example on how to capture a picture with
# python-v4l2capture. It waits between starting the video device and
# capturing the picture, to get a good picture from cameras that
# require a delay to get enough brightness. It does not work with some
# devices that require starting to capture pictures immediatly when
# the device is started.
#
# python-v4l2capture
# Python extension to capture video with video4linux2
#
# 2009, 2010, 2011 Fredrik Portstrom, released into the public domain
# 2011, Joakim Gebart
# 2013, Tim Sheerman-Chase
# See README for license

import Image
import select
import time
import v4l2capture

# Open the video device.
video = v4l2capture.Video_device("/dev/video0")

# Suggest an image size to the device. The device may choose and
# return another size if it doesn't support the suggested one.
size_x, size_y = video.set_format(1280, 1024)

# Create a buffer to store image data in. This must be done before
# calling 'start' if v4l2capture is compiled with libv4l2. Otherwise
# raises IOError.
video.create_buffers(1)

# Start the device. This lights the LED if it's a camera that has one.
video.start()

# Wait a little. Some cameras take a few seconds to get bright enough.
time.sleep(2)

# Send the buffer to the device.
video.queue_all_buffers()

# Wait for the device to fill the buffer.
select.select((video,), (), ())

# The rest is easy :-)
image_data = video.read()
video.close()
image = Image.fromstring("RGB", (size_x, size_y), image_data)
image.save("image.jpg")
print "Saved image.jpg (Size: " + str(size_x) + " x " + str(size_y) + ")"
