#!/usr/bin/python
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

def exc_get(f, *args):
	try:
		return f(*args)
	except Exception, e:
		return str(e)

file_names = [x for x in os.listdir("/dev") if x.startswith("video")]
file_names.sort()
for file_name in file_names:
	path = "/dev/" + file_name
	print path
	try:
		video = v4l2capture.Video_device(path)
		driver, card, bus_info, capabilities = video.get_info()
		print "\tDriver:", driver
		print "\tCard:", card
		print "\tBus-info:", bus_info
		print "\tCapabilities:", ", ".join(capabilities)
		width, height, fourcc = video.get_format()
		print "\tWidth:", width
		print "\tHeight:", height
		print "\tFourcc:", fourcc
		for cap in exc_get(video.get_framesizes, fourcc):
			print "\tFrame-size:", cap
		for cap in exc_get(video.get_frameintervals, fourcc, width, height):
			print "\tFrame-interval:", cap
		print "\tAuto-white-balance:", exc_get(video.get_auto_white_balance)
		print "\tWhite-balance-temperature:", exc_get(video.get_white_balance_temperature)
		print "\tAuto-exposure:", exc_get(video.get_exposure_auto)
		print "\tExposure-absolute:", exc_get(video.get_exposure_absolute)
		print "\tAuto-focus:", exc_get(video.get_focus_auto)
	finally:
		video.close()
