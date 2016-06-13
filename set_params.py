#!/usr/bin/python
#
# python-v4l2capture
import v4l2capture


# Open the video device.
video = v4l2capture.Video_device("/dev/video0")

video.set_brightness(20)
print video.get_brightness()
video.set_saturation(10)
print video.get_saturation()
video.set_contrast(20)
print video.get_contrast()
video.set_hue(30)
print video.get_hue()
video.set_gamma(80)
print video.get_gamma()
video.set_sharpness(4)
print video.get_sharpness()
video.set_zoom(4)
print video.get_zoom()
    
video.close()
