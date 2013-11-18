
import videolive

if __name__=="__main__":
	inManager = videolive.Video_in_manager()
	print inManager

	devs = inManager.list_devices()
	print devs

	firstDev = open(devs[0][0])
	print firstDev

	