
import videolive, time

if __name__=="__main__":
	inManager = videolive.Video_in_manager()
	print inManager

	devs = inManager.list_devices()
	print devs

	if len(devs) == 0:
		print "No source devices detected"
		exit(0)

	inManager.open(devs[0][0])
	
	time.sleep(1)
	inManager.start(devs[0][0])
	count = 0	

	while count < 10:
		time.sleep(0.01)
		frame = inManager.get_frame(devs[0][0])
		if frame is None: continue
		print len(frame[0]), frame[1]
		count += 1
		
	inManager.stop(devs[0][0])

	time.sleep(1)

	inManager.close(devs[0][0])

	time.sleep(1)

	del inManager

