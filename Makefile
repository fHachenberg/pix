OPENCV_LIB=-lopencv_core -lopencv_photo -lopencv_imgproc -lopencv_highgui

cmdlinedriver:
	g++ -Wall -O3 -I . pix.cpp stateList.cpp cmdline-driver/cmdlinetool.cpp $(OPENCV_LIB) -lc -o pix

PYWRAPPER_OBJ_COMPILE_FLAGS=-Wall -O2 -fPIC
PYTHON_INCDIR=/usr/include/python2.7/
PYTHON_LIB=-lpython2.7
BOOST_PYTHON_LIB=-lboost_python-py27	
WRAPPER_OBJ=wrapper_obj/boost_python_export.o wrapper_obj/pix.o wrapper_obj/stateList.o wrapper_obj/mat_conversion.o
wrapper_obj:
	mkdir wrapper_obj
wrapper_obj/pix.o: pix.cpp | wrapper_obj
	g++ $(PYWRAPPER_OBJ_COMPILE_FLAGS) -I . pix.cpp -c -o wrapper_obj/pix.o
wrapper_obj/stateList.o: stateList.cpp | wrapper_obj
	g++ $(PYWRAPPER_OBJ_COMPILE_FLAGS) -I . stateList.cpp -c -o wrapper_obj/stateList.o
wrapper_obj/boost_python_export.o: boost-python-wrapper/boost_python_export.cpp | wrapper_obj
	g++ $(PYWRAPPER_OBJ_COMPILE_FLAGS) -I . -I $(PYTHON_INCDIR) boost-python-wrapper/boost_python_export.cpp -c -o wrapper_obj/boost_python_export.o
wrapper_obj/mat_conversion.o: boost-python-wrapper/mat_conversion.cpp | wrapper_obj
	g++ $(PYWRAPPER_OBJ_COMPILE_FLAGS) -I . -I $(PYTHON_INCDIR) boost-python-wrapper/mat_conversion.cpp -c -o wrapper_obj/mat_conversion.o
wrapper_clean:
	rm -rf wrapper_obj
pythonwrapper: $(WRAPPER_OBJ)
	g++ -shared -Wl,-soname,libpix.so $(WRAPPER_OBJ) $(OPENCV_LIB) $(BOOST_PYTHON_LIB) $(PYTHON_LIB) -lc -o pix.so

clean: wrapper_clean
all: cmdlinedriver pythonwrapper

.PHONY: cmdlinedriver wrapper_clean pythonwrapper clean all

