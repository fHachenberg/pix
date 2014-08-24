===================================================================
License
=================================================================== 
Copyright (c) 2013, Timothy Gerstner, All rights reserved.

This code is part of the prototype C++ implementation of our 
paper/ my thesis.

Public repository: https://github.com/timgerst/pix
Project Webpage:  http://www.research.rutgers.edu/~timgerst/

This code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this code.  If not, see <http://www.gnu.org/licenses/>.

====================================================================
Disclaimer
====================================================================
This code is part of a research project. While effort has been made
to keep it clean and readable, it may still contain bugs, misleading
comments, and poorly named variables. This code is released AS IS, 
without any warranty or support. For questions related to its 
implementation, please see the corresponding thesis and publications.

====================================================================
Required Libraries:
====================================================================
OpenCv 2.3.0: http://opencv.willowgarage.com/wiki/

If Cinder-GUI is used:
Cinder 0.8.4: http://libcinder.org/

If Python-Wrapper is created:
Boost Python http://www.boost.org/

====================================================================
Description
====================================================================
This code is an implementation of the algorithm described in the
"Pixelated Abstraction" Master's Thesis and the related publications:
"Pixelated Image Abstraction" and "Pixelated Image Abstraction with
Integrated User Constraints". The base algorithm is contained in the
pix.h/.cpp files and requires methods/variables in the utility.h,
const.h, and statelist.h/.cpp files to run. The PixUI class found in
pixui.h/.cpp is an interface for the algorithm, but is not required 
to run the algorithm itself. 

====================================================================
Build
====================================================================
There's Makefile present to create the commandline driver and/or the
Python wrapper.
The commandline driver is only dependent on OpenCV

    make cmdlinedriver

To build the Python wrapper, you will have to provide the path where
pyconfig.h resides. 
Also you will have to choose the correct Python and Boost Python 
library versions (2.x or 3.x)

Examples
--------

Python 2.7 version, Ubuntu system

    make pythonwrapper
    
Python 3.4 version, Ubuntu system

    make pythonwrapper PYTHON_INCDIR=/usr/include/python3.4m/ \
         PYTHON_LIB=-lpython3.4 BOOST_PYTHON_LIB=-lboost_python-py34
