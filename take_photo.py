#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from PIL import Image
import select
import time
import datetime
import v4l2capture

video = v4l2capture.Video_device("/dev/video0")
size_x, size_y = video.set_format(1280, 1024)
video.create_buffers(1)
video.start()
time.sleep(2)
video.queue_all_buffers()
select.select((video,), (), ())
image_data = video.read()
video.close()
image = Image.frombytes("RGB", (size_x, size_y), image_data)
image.save('/tmp/{0}.jpg'.format(
    datetime.datetime.now().strftime('%Y%m%dT%H%M%S')))
