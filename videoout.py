
import videolive, time
import numpy as np

if __name__=="__main__":
	outManager = videolive.Video_out_manager()
	print outManager

	devs = outManager.list_devices()
	print devs

	if len(devs) == 0:
		print "No source devices detected"
		exit(0)

	outManager.open(devs[0], "RGB24", 640, 480)

	imgLen = 640 * 480 * 3
	img = np.zeros(shape=(imgLen,), dtype=np.uint8)

	for i in range(imgLen):
		if (i % 500) > 250:
			img[i] = np.random.randint(0, 255)
		else:
			img[i] = 128
	
	while(1):
		outManager.send_frame(devs[0], str(img.tostring()), "RGB24", 640, 480)

		time.sleep(0.1)
                
