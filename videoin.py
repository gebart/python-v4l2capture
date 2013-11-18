
import videolive, time

if __name__=="__main__":
	inManager = videolive.Video_in_manager()
	print inManager

	devs = inManager.list_devices()
	print devs

	if len(devs) == 0:
		print "No source devices detected"
		exit(0)

	firstDev = inManager.open(devs[0][0])
	print firstDev

	for i in range(10):
		time.sleep(1)
