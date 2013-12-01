
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

        img = np.zeros(shape=(640 * 480 * 3,), dtype=np.uint8)
	
        for i in range(100):
                outManager.send_frame(devs[0], str(img.tostring()), "RGB24", 640, 480)

                time.sleep(0.1)
                
