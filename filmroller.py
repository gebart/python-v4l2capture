#!/usr/bin/env python
from select import select
from time import time
from os.path import exists
from os import listdir, makedirs
from ConfigParser import RawConfigParser
from Image import frombytes, open as fromfile
from ImageTk import PhotoImage
from ImageOps import invert, autocontrast, grayscale
from Tkinter import Frame, Button, Tk, Label, Canvas, BOTH, TOP, Checkbutton, OptionMenu, StringVar, BooleanVar, Menu
from v4l2capture import Video_device

'''
webcam liveview and single picture capture program. this program shows a
picture in low resolution. when triggered (by 'space', 'enter' or the button)
it switches the webcam to a high resolution to take a picture and stores it.
after that it switches back to the liveview mode in low resulution.

the filename for storage is counted up by pic no and role (which is aa, ab,
ac...). it tries to determine the next pic no to not overwrite existing files.
is stores the scans in the current directory.

this program is good to be used with scanners for analog film rolls where you
manually position the picture with a live view and scan in highest possible
resolution.

it needs tkinter (see http://effbot.org/tkinterbook/tkinter-index.htm), pil
image, imageops and imagetk and the famous v4l2capture.

TODO:
- set v4l properties (contrast, hue, sat, ..)
- get event from usb dev
- reduce redundant code
- different target dir
- show countdown during take
'''

def ascii_increment(val):
	' count aa, ab, ac ... '
	a = ord('a')
	i = (ord(val[0]) - a) * 26 + (ord(val[1]) - a)
	i += 1
	return chr(a + i / 26) + chr(a + i % 26)

class Cap(Frame):
	def __init__(self):
		' set defaults, create widgets, bind callbacks, start live view '
		# go!
		self.root = Tk()
		self.root.bind('<Destroy>', self.do_stop_video)
		self.root.bind('<space>', self.do_single_shot)
		self.root.bind('<Return>', self.do_single_shot)
		self.root.bind('<Left>', self.do_change_rotation_clockwise)
		self.root.bind('<Right>', self.do_change_rotation_counterclockwise)
		self.root.bind('<Up>', self.do_change_rotation_reset)
		self.root.bind('q', self.do_quit)
		self.root.bind('i', self.do_toggle_invert)
		self.root.bind('a', self.do_toggle_auto)
		self.root.bind('b', self.do_toggle_bw)
		self.root.bind('x', self.do_quit)
		self.root.bind('<Button-3>', self.do_single_shot)
		# config:
		self.config = RawConfigParser()
		self.config.read('filmroller.conf')
		if not self.config.has_section('global'):
			self.config.add_section('global')
		self.video = None
		self.invert = BooleanVar(name='invert')
		self.invert.set(self.config_get('invert', True))
		self.invert.trace('w', self.do_configure)
		self.bw = BooleanVar(name='bw')
		self.bw.set(self.config_get('bw', False))
		self.bw.trace('w', self.do_configure)
		self.auto = BooleanVar(name='auto')
		self.auto.set(self.config_get('auto', True))
		self.auto.trace('w', self.do_configure)
		self.videodevice = StringVar(name='videodevice')
		dev_names = sorted(['/dev/{}'.format(x) for x in listdir('/dev') if x.startswith('video')])
		d = self.config_get('videodevice', dev_names[-1])
		if not d in dev_names:
			d = dev_names[-1]
		self.videodevice.set(d)
		self.videodevice.trace('w', self.do_configure)
		#
		self.menu = Menu(self.root)
		self.root.config(menu=self.menu)
		filemenu = Menu(self.menu)
		self.menu.add_cascade(label=self.videodevice.get(), menu=filemenu, )
		for n in dev_names:
			filemenu.add_command(label=n, )
		#filemenu.add_separator()
		#
		self.path = 'filmroller'
		if not exists(self.path):
			makedirs(self.path)
		self.degree = 0
		#
		Frame.__init__(self, self.root)
		self.grid()
		self.x_canvas = Canvas(self, width=640, height=640, )
		self.x_canvas.pack(side='top')
		self.x_canvas.bind('<Button-1>', self.do_change_rotation)
		self.x_invert = Checkbutton(self, text='Invert', variable=self.invert)
		self.x_invert.pack(side='left')
		self.x_bw = Checkbutton(self, text='B/W', variable=self.bw)
		self.x_bw.pack(side='left')
		self.x_auto = Checkbutton(self, text='Auto', variable=self.auto)
		self.x_auto.pack(side='left')
		self.x_restart_video = OptionMenu(self, self.videodevice, *dev_names, command=self.restart_video)
		self.x_restart_video.pack(side='left')
		self.x_first_role = Button(self, text='First role', command=self.do_first_role)
		self.x_first_role.pack(side='left')
		self.x_filename = Label(self)
		self.x_filename.pack(side='left')
		self.x_inc_role = Button(self, text='Next role', command=self.do_inc_role)
		self.x_inc_role.pack(side='left')
		self.x_single_shot = Button(self, text='Take!', command=self.do_single_shot)
		self.x_single_shot.pack(side='right')
		self.do_first_role()
		self.do_start_video()

	def do_toggle_invert(self, *args):
		self.invert.set(not self.invert.get())

	def do_toggle_auto(self, *args):
		self.auto.set(not self.auto.get())

	def do_toggle_bw(self, *args):
		self.bw.set(not self.bw.get())

	def do_change_rotation_clockwise(self, *args):
		self.degree = -90

	def do_change_rotation_counterclockwise(self, *args):
		self.degree = 90

	def do_change_rotation_reset(self, *args):
		self.degree = 0

	def do_change_rotation(self, event):
		' determine where the image was clicked and turn that to the top '
		if event.x < 200:
			self.do_change_rotation_clockwise()
		elif event.x > 640 - 200:
			self.do_change_rotation_counterclockwise()
		else:
			self.do_change_rotation_reset()

	def config_get(self, name, default):
		' read a configuration entry, fallback to default if not already stored '
		if not self.config.has_option('global', name):
			return default
		if isinstance(default, bool):
			return self.config.getboolean('global', name)
		else:
			return self.config.get('global', name)

	def do_configure(self, name, mode, cbname):
		' change a configuration entry '
		if cbname == 'w':
			value = getattr(self, name).get()
			self.config.set('global', name, str(value))
			self.config.write(open('filmroller.conf', 'w'))

	def do_first_role(self, *args):
		' jump back to first role '
		self.role = 'aa'
		self.serial = 0
		self.inc_picture()

	def inc_picture(self):
		' increment the picture number, jump over existing files '
		self.filename = '{}/scanned.{}-{:04}.jpg'.format(self.path, self.role, self.serial, )
		while exists(self.filename):
			self.serial += 1
			self.filename = '{}/scanned.{}-{:04}.jpg'.format(self.path, self.role, self.serial, )
		self.root.title('filmroller - ' + self.filename)
		self.x_filename['text'] = self.filename
		self.root.title('filmroller - ' + self.filename)

	def do_inc_role(self, *args):
		' increment to next role '
		self.serial = 0
		self.role = ascii_increment(self.role)
		self.inc_picture()

	def set_pauseimage(self):
		' show pause image (during shot) '
		self.image = fromfile('filmroller.pause.png')
		self.image.thumbnail((self.previewsize['size_x'], self.previewsize['size_y'], ), )
		self.photo = PhotoImage(self.image)
		self.x_canvas.create_image(640/2, 640/2, image=self.photo)

	def do_quit(self, *args):
		' exit program '
		self.root.destroy()

	def do_stop_video(self, *args):
		' stop video and release device '
		if self.video is not None:
			self.video.stop()
			self.video.close()
			self.video = None

	def restart_video(self, *args):
		' restart video (if device changes or hangs) '
		self.do_stop_video()
		self.root.after(1, self.do_start_video)

	def do_start_video(self, *args):
		' init video and start live view '
		if self.video is None:
			self.video = Video_device(self.videodevice.get())
			_, _, self.fourcc = self.video.get_format()
			caps = sorted(self.video.get_framesizes(self.fourcc), cmp=lambda a, b: cmp(a['size_x']*a['size_y'], b['size_x']*b['size_y']))
			self.previewsize, self.highressize = caps[0], caps[-1]
			self.previewsize['size_x'], self.previewsize['size_y'] = self.video.set_format(
				self.previewsize['size_x'], self.previewsize['size_y'], 0, 'MJPEG')
			try: self.video.set_auto_white_balance(True)
			except: pass
			try: self.video.set_exposure_auto(True)
			except: pass
			try: self.video.set_focus_auto(True)
			except: pass
			self.video.create_buffers(30)
			self.video.queue_all_buffers()
			self.video.start()
			self.root.after(1, self.do_live_view)
			#self.x_canvas.width=640
			#self.x_canvas.height=480
			#self.x_canvas.pack(side='top')
			self.degree = 0

	def do_live_view(self, *args):
		' show single pic live view and ask tk to call us again later '
		if self.video is not None:
			select((self.video, ), (), ())
			data = self.video.read_and_queue()
			self.image = frombytes('RGB', (self.previewsize['size_x'], self.previewsize['size_y']), data)
			if self.invert.get():
				self.image = invert(self.image)
			if self.bw.get():
				self.image = grayscale(self.image)
			if self.auto.get():
				self.image = autocontrast(self.image)
			if self.degree:
				self.image = self.image.rotate(self.degree)
			self.photo = PhotoImage(self.image)
			self.x_canvas.create_image(640/2, 640/2, image=self.photo)
			self.root.after(3, self.do_live_view)

	def do_single_shot(self, *args):
		' do a high res single shot and store it '
		def _go():
			self.video = Video_device(self.videodevice.get())
			try:
				self.highressize['size_x'], self.highressize['size_y'] = self.video.set_format(
					self.highressize['size_x'], self.highressize['size_y'], 0, 'MJPEG')
				try: self.video.set_auto_white_balance(True)
				except: pass
				try: self.video.set_exposure_auto(True)
				except: pass
				try: self.video.set_focus_auto(True)
				except: pass
				self.video.create_buffers(7)
				self.video.queue_all_buffers()
				self.video.start()
				stop_time = time() + 3.0
				# wait for auto
				while stop_time >= time():
					select((self.video, ), (), ())
					self.update_idletasks()
					data = self.video.read_and_queue()
				image = frombytes('RGB', (self.highressize['size_x'], self.highressize['size_y'], ), data)
				if self.invert.get():
					image = invert(image)
				if self.bw.get():
					image = grayscale(image)
				if self.auto.get():
					image = autocontrast(image)
				if self.degree:
					image = image.rotate(self.degree)
				image.save(self.filename)
				self.inc_picture()
				self.root.bell()
				self.video.stop()
			finally:
				self.video.close()
				self.video = None
			self.root.after(1, self.do_start_video)
		self.do_stop_video()
		self.set_pauseimage()
		self.update_idletasks()
		self.root.after(1, _go)


def main():
	' main start point of the program '
	app = Cap()
	app.mainloop()

if __name__ == '__main__':
	from sys import argv
	main(*argv[1:])
# vim:tw=0:nowrap
