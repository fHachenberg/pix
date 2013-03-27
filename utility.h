/* 
Copyright (c) 2013, Timothy Gerstner, All rights reserved.

This code is part of the prototype C++ implementation of our paper/ my thesis.

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

Author: Timothy Gerstner, timgerst@cs.rutgers.edu

Description: This file contains several utility functions used by the Pix and Pixgui classes

*/
#pragma once

#include "opencv2/opencv.hpp"

namespace pix_research
{


//implements a gaussian function with std "sigma" and mean "mean"
inline float gaussian(float x, float sigma, float mean)
{
	return exp((x-mean)*(x-mean)/(-2.0f*sigma*sigma))/sqrt(6.28319*sigma*sigma);
}

//returns a random vector of unit length
inline cv::Vec3f randVec()
{
	cv::Vec3f p((float) (rand() % 100), 
              (float) (rand() % 100),
              (float) (rand() % 100));
	p *= 1.0f/(float)norm(p);
	return p;
}

//returns a random float in the range [0,1]
inline float randFloat()
{
	return (float)rand()/(float)RAND_MAX;
}

//returns true if string "fullstring" ends with any of the strings in the 
//list "endings"
inline bool endsWith(std::string const &fullString, 
                     std::vector<std::string> const &endings)
{
	for(std::vector<std::string>::const_iterator nExt = endings.begin(); nExt != endings.end(); ++nExt)
	{
		if (fullString.length() < (*nExt).length()) continue;
		if(0 == fullString.compare(fullString.length() - (*nExt).length(), 
                               (*nExt).length(), (*nExt)))
			return true;
	}
		return false;
}

//Converts an RGB color to the L*a*b* color space
//(note: not a very efficient way but ensures the algorithm consistently uses
// the openCV implementation of this color conversion)
inline cv::Vec3f rgb2lab(cv::Vec3f rgb)
{
	cv::Mat holder(cv::Size(1,1),CV_32FC3);
	holder.at<cv::Vec3f>(0,0) = rgb;
	cv::Mat holder2;
	cvtColor(holder, holder2,CV_RGB2Lab);
	return holder2.at<cv::Vec3f>(0,0);
}
//Converts an L*a*b* color to the rgb color space
//(note: not a very efficient way but ensures the algorithm consistently uses
// the openCV implementation of this color conversion)
inline cv::Vec3f lab2rgb(cv::Vec3f lab)
{
	cv::Mat holder(cv::Size(1,1),CV_32FC3);
	holder.at<cv::Vec3f>(0,0) = lab;
	cv::Mat holder2;
	cvtColor(holder, holder2,CV_Lab2RGB);
	return holder2.at<cv::Vec3f>(0,0);
}

}