cmdlinetool:
	g++ -O3 -I . pix.cpp stateList.cpp cmdlinetool.cpp -l opencv_core -l opencv_photo -l opencv_imgproc -l opencv_highgui

all: cmdlinetool

