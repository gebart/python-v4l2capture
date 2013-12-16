
import videolive, time, random
import numpy as np

if __name__=="__main__":
	outManager = videolive.Video_out_file_manager()
	print outManager

	outManager.open("test.wmv", "RGB24", 640, 480)

	w = 800
	h = 600

	imgLen = w * h * 3
	img = np.ones(shape=(imgLen,), dtype=np.uint8) * 0
	for i in range(imgLen):
		if (i % 3) == 0:
			img[i] = 0xff
		if (i % 3) == 1:
			img[i] = random.randint(0,255)

	for frNum in range(200):
		#img = np.random.randint(0, 255, size=(imgLen,))
		#for i in range(imgLen):
		#	if (i % 500) <= 250:
		#		img[i] = 128

		print "Frame", frNum
		outManager.send_frame("test.wmv", str(img.tostring()), "RGB24", w, h)

		time.sleep(0.01)
                
