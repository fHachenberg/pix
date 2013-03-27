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

Description: An implementation of the algorithm described in my
  Pixelated Abstraction thesis and related publications. Please see
  the thesis for details. 
*/

#pragma once

#include "opencv2/opencv.hpp"
#include "stateList.h"
#include "utility.h"
#include <vector>
#include <list>

using namespace pix_research;

class Pix {

 public:
  //Constructs a new Pix object from an image and the output spatial and
  //palette size. Additional inputs (non default parameters, weights) should
  //be set using relevant methods before initialization. The input image
  //should be an 8U, 3 channel rgb image.
  Pix(const cv::Mat& img_input, int w, int h, int p);
  
  //Constructs a new Pix Object from a file a .pix file from a previous session
  //Do not need to call initialize if using this constructor.
  Pix(std::string filename);
  
  //
  ~Pix();

  //Initializes the algorithm based on the current parameters.
  void Initialize();

  //Saves the current state to a file. filename is the full path of the file.
  void SaveToFile(std::string filename);

  //Performs a single iteration of the algorithm. Does nothing if
  //converged_flag_ is set to true.
  void Iterate();

  //associates superpixels with colors in the palette
  void AssociatePalette();
  
  //returns the current palette, subclusters are treated as a single color.
  std::vector<cv::Vec3f> GetPalette();

  //returns the output image as an 8U, rgb image
  void GetOutputImage(cv::Mat& img);

  //returns the input image with the superpixel segmentation visualized
  void GetRegionImage(cv::Mat& img);
 
  //Sets the input weights. Only call before initialization.
  inline void set_input_weights(cv::Mat& w){w.copyTo(input_weights_);}
 
  //Sets the laplacian factor used to smooth the superpixel positions.
  inline void set_laplacian_factor(float f){smooth_pos_factor_ = f;}
 
  //Sets the bilateral filter parameters used to smooth the superpixel colors.
  inline void SetBilateralParams(float sigCol, float sigPos){
    sigma_color_ = sigCol; sigma_position_ = sigPos;}
  
  //Sets the weight factor used to normalize Color and Spatial components in
  //the SLIC distance metric. Value should be in the range [0,1].
  inline void setSlicFact(float f){slic_factor_ = f;}
  
  //Sets the saturation value used in the output. Values >1 increase saturation
  //and values <1 decrease saturation.
  inline void SetSaturation(float f){GetCurrentState()->saturation = f;}
  
  //Sets the lock of a the color at the given index. Locked colors do not 
  //update during iteration.
  inline void SetColorLock(int index, bool locked){
    GetCurrentState()->locked_colors[index] = locked;
    converged_flag_ = false;
  }

  //Gets the lock state of the color at the given index.
  inline bool GetColorLock(int index) { 
    return GetCurrentState()->locked_colors[index];
  }

  //Sets the constraints of the output pixel at the given location in the 
  //output image.
  inline void SetPixelConstraints(cv::Vec2i pixel, const std::list<int>& constraints) {
    GetCurrentState()->pixel_constraints[vec2idx(pixel)] = 
    std::list<int>(constraints);
    converged_flag_ = false;
  }
  
  //Sets the color in the palette at the given index to the given color.
  inline void SetColor(int index, cv::Vec3f color) {
    cv::Vec3f lab = rgb2lab(color);
	  lab[1] /= GetCurrentState()->saturation;
	  lab[2] /= GetCurrentState()->saturation;
	
	  GetCurrentState()->palette[index] = rgb2lab(color);
    converged_flag_ = false;
  }

  //sets the color in the palette at the given index to the color value of the
  //superpixel at the given location
  inline void SetColorFromSP(int index, cv::Vec2i superpixel) {
    cv::Vec3f superpixel_color = 
    GetCurrentState()->superpixel_color.at<cv::Vec3f>(superpixel[1],
                                                      superpixel[0]);
	  GetCurrentState()->palette[index] = superpixel_color;
    converged_flag_ = false;
  }

  //returns the input image as an 8U, rgb image
  inline void GetInputImage(cv::Mat& img) { cvtColor(input_img_, img, CV_Lab2RGB);}

  //returns an 8U, rgb image representing the superpixel color values
  inline void GetSuperpixelImage(cv::Mat& img) {
    cv::Mat rgb;
	  cvtColor(GetCurrentState()->superpixel_color, rgb, CV_Lab2RGB);
	  rgb.convertTo(img, CV_8UC3, 255.0);
  }

  //returns a vector the same size as the palette, indicating whether
  //each color in the palette is currently locked
  inline std::vector<bool> get_locked_colors() {
    return GetCurrentState()->locked_colors;
  }

  //returns the current pixel constraints. Pixels indexed into the
  //vector in row major order.
  inline std::vector<std::list<int>> get_pixel_constraints() {
    return GetCurrentState()->pixel_constraints;
  }

  //returns true if the algorithm has converged
  inline bool hasConverged(){return converged_flag_;}

  //returns the current iteration number
  inline int get_iteration(){return GetCurrentState()->iteration;}
  
  //returns the width of the input image
  inline int get_input_width(){return input_width_;}

  //returns the height of the input image
  inline int get_input_height(){return input_height_;}

  //returns the width of the output image
  inline int get_output_width(){return output_width_;}

  //returns the height of the output iamge
  inline int get_output_height(){return output_height_;}

  //returns the maximum palette size
  inline int get_max_palette_size(){return max_palette_size_;}

  //reloads the last saved state. Does nothing if no previous state exists.
  inline void Undo() {
    state_list_->stepBack();
	  UpdateSuperpixelMapping();
  }

  //reloads the next saved state. Dones nothing if no such state exists.
  inline void Redo() {
    state_list_->stepForward();
	  UpdateSuperpixelMapping();
  }

  //saves the current state. This removes any existing states after the current
  //state.
  inline void SaveState() {
    state_list_->push_copy();
  }

  //converts the index value to the equivelant output pixel position
  inline cv::Vec2i idx2vec(int index) {
	  return cv::Vec2i( index % output_width_, 
      (int)floor(((float)index) / output_width_));
  }

  //converts the ouput pixel position to the equivelant 1D array index
  inline int vec2idx(cv::Vec2i v) {
	  return v[0] + output_width_*v[1];
  }

 private: 
	//Updates the mapping of input pixels to superpixels
  void UpdateSuperpixelMapping();

  //Updates superpixel color and spatial values
  void UpdateSuperpixelMeans();

  //Smooths the superpixel positions using laplacian smoothing.
	void SmoothSuperpixelPositions();

  //Smooths the superpixel colors using a bilateral filter. 
	void SmoothSuperpixelColors();
	
	//Refines the palette based on superpixel association to colors
	float RefinePalette();
	
  //Checks to see if any color in the palette needs to be split. Calls 
  //SplitColor() on any such colors. If the maximum palette size is reached, 
  //the palette_maxed_flag_ flag is set to true and the method calls 
  //CondensePalette(). If palette_maxed_flag_ flag == true, does nothing.
	void ExpandPalette();

  //Splits the color at the given index's superpixels into two independent 
  //superpixels, each represented by subsuperpixels
	void SplitColor(int pair_index);

	//Removes subsuperpixels in the palette and represents each color as a single
  //superpixel (no subsuperpixels)
	void CondensePalette();

  // returns the largest Eigenvector and Eigenvalue of the color in the palette
  //at the given index.
  std::pair<cv::Vec3f, float> GetMaxEigen(int palette_index);

  //returns the palette with subsuperpixels set to their weighted average
	std::vector<cv::Vec3f> GetAveragedPalette();

	//returns the current algorithm state
	inline pixState * GetCurrentState(){return state_list_->getCur();}

  int output_width_, output_height_, input_width_, input_height_, max_palette_size_;
	int range_;
	cv::Mat input_img_, output_img_;
	cv::Mat input_weights_, superpixel_weights_;
	cv::Mat region_map_;
	std::vector<std::vector<cv::Vec2i> > region_list_;
	std::vector<std::vector<float> > prob_oc_; 
	std::vector<std::vector<float> > prob_co_; 
	float prob_o_;
	float slic_factor_; 
	float smooth_pos_factor_; 
	float temperature_; 
	float sigma_color_, sigma_position_; 
	bool converged_flag_, palette_maxed_flag_; 
	stateList * state_list_; 
	
};