
import videolive, time, random
import numpy as np
import scipy.misc as misc

if __name__=="__main__":
	outManager = videolive.Video_out_file_manager()
	print outManager

	lena = misc.imread("Lenna.png")
	print lena.shape
	w = lena.shape[1]
	h = lena.shape[0]

	outManager.open("test.wmv", "BGR24", 640, 480)
	outManager.set_frame_rate("test.wmv", 5)
	imgLen = w * h * 3
	#img = np.ones(shape=(imgLen,), dtype=np.uint8) * 0
	#for i in range(imgLen):
	#	if (i % 3) == 0:
	#		img[i] = 0xff
	#	if (i % 3) == 1:
	#		img[i] = random.randint(0,255)

	for frNum in range(200):
		#img = np.random.randint(0, 255, size=(imgLen,))
		#for i in range(imgLen):
		#	if (i % 500) <= 250:
		#		img[i] = 128

		print "Frame", frNum
		outManager.send_frame("test.wmv", str(lena.tostring()), "RGB24", w, h)

		time.sleep(0.01)
                
