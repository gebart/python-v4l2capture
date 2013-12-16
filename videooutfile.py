
import videolive, time
import numpy as np

if __name__=="__main__":
	outManager = videolive.Video_out_file_manager()
	print outManager

	outManager.open("test.wmv", "RGB24", 640, 480)

	imgLen = 640 * 480 * 3
	img = np.zeros(shape=(imgLen,), dtype=np.uint8)

	for frNum in range(1000):
		for i in range(imgLen):
			if (i % 500) > 250:
				img[i] = np.random.randint(0, 255)
			else:
				img[i] = 128
		
		print "Frame", frNum
		outManager.send_frame("test.wmv", str(img.tostring()), "RGB24", 640, 480)

		time.sleep(0.1)
                
