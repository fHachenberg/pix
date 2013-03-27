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

Description: A UI to provide user constraints to the algorithm described
in my Thesis and related publications. See the related guide on the
project webpage for a description of how to use the UI.
*/


#pragma once

#include "cinder\app\AppBasic.h"
#include "cinder\gl\Texture.h"
#include "cinder\params\Params.h"
#include "pix.h"
#include "interface.h"
#include "cinder\Text.h"
#include <queue>

using namespace ci;
using namespace ci::app;
using namespace ci::params;
using namespace pix_research;

class PixUI : public AppBasic
{
 public: 
  PixUI(){};

  ~PixUI();

  //overloaded function. Prepares window settings
  void prepareSettings(Settings * settings);

  //sets up the interface into the NODATA state
  void setup();

  //updates the interface. iterates the algorithm.
  void update();

  //draws the current view based on state
  void draw();

  //callback to handle key up events
  void keyUp(KeyEvent event);

  //callback to handle key down events
  void keyDown(KeyEvent event);

  //callback to handle mouse down events
  void mouseDown(MouseEvent e);

  //Callback to handle mouse wheel events
  void mouseWheel(MouseEvent e);

  //Callback to handle mouseDrag events
  void mouseDrag(MouseEvent e);

 private:

  enum MOUSECLICK{LEFT, RIGHT};
  enum STATE{NODATA,WEIGHTING,ITERATING, EDIT};

  //Causes the algorithm to iterate. If in the Weighting mode, initializes
  //the Pix object.
  void Process();

  //calls Pix object functions to reassociate superpixels to colors in the
  //palette
  void Reassociate();

  //initializes the anttweakbar
  void InitializeBar();

  //updates the anttweakbar to options relevant to the current state
  void UpdateBar();

  //draws the interface background
  void DrawBackground();

  //draws the interface in the weighting state
  void DrawWeightingMode();

  //draws the interface in the iterating state
  void DrawIteratingMode();

  //draws the interface in the editting state
  void DrawEditMode();

  //draws the current palette
  void DrawPalette();

  //draws the preview window
  void DrawPreview();

  //draws the current message to the interface
  void DrawMessage();

  //sets the message to display on the interface
  void set_message(std::string message);

  //sets the current state of the interface
  void SetState(PixUI::STATE s);

  //returns the area on the interface of the main image view
  //Note: this already accounts for the aspect ratio of the image.
  Rectf GetImgPos();

  //returns the area on the interface of the preview image
  //Note: this already accounts for the aspect ratio of the image and
  //centers it within the preview window
  Rectf GetPreviewPos();

  //returns the area on the interface of the color in the palette with the given
  //index. This method automatically calculates the appropriate size and 
  //position based on the palette window and the number of colors in the palette
  Rectf GetPalettePos(int idx);

  //transforms the click into the coordinates of an image of width w and height 
  //h positions in the interface using GetImgPos(). It is up to the code calling
  //this transformation to check that the returned point is within the image. 
  cv::Vec2i GetImgClick(ci::Vec2i click, int w, int h);

  //returns the index of a color in the palette based on the position of a mouse
  //click on the interface. Returns -1 if the click is not within the bounds of 
  //any color. 
  int GetPaletteClick(ci::Vec2i click);

  //applies pixel constraints based on the current paint queue
  void PaintPixels();

  //draws weights to the weight map based on the current paint
  //queue
  void PaintWeight();

  //returns a list of all pixel positions within the brush radius
  //from the given center position.
  std::list<cv::Vec2i> GetPixelBrush(cv::Vec2i center);

  //Sets the currently selected color to the value chosen by the user
  void SetColor();

  //Updates the visualization of the weight map.
  void UpdateWeightImg();

  //opens a dialog to load a new Project from a source image
  void NewProj();

  //opens a dialog to load a previous project 
  void LoadProj();

  //resets the interface. Will retain any previously draw weight maps unless it
  //is called while in the weighting state.
  void Reset();

  //opens a dialog to saves the curernt Project as a .pix file
  void SaveProj();

  //saves the current output as a PNG image
  void SaveImg();

  //Creates a dialog box to open a file. exts is a list of valid extensions.
  //Returns the path of the file to be opened.
  std::string OpenFileDialog(std::vector<std::string> exts);

  //creates a dialog box to save a file. exts is a list of valid extensions. 
  //Returns the path of the file to be saved. If the path does not contain an
  //extension, the first extension on the list is automatically added to the 
  //end of the file path.
  std::string SaveFileDialog(std::vector<std::string> exts);

  Pix * pix_;
  gl::Texture input_img_, output_img_, average_img_, weight_img_;
  gl::Texture lock_img_, select_img_, palette_label_, preview_label_;
  int output_width_, output_height_, max_palette_size_;
  int previous_width_, previous_height_;
  cv::Mat input_weights;
  STATE state_;
  interface bar_;
  bool fixed_are_visible_;
  bool output_img_is_outdated_;
  bool weight_img_is_outdated_;
  bool output_is_hidden_;
  bool superpixels_are_visible_;
  float slic_factor_, smooth_factor_, sigma_position_, sigma_color_;
  float saturation_, previous_saturation_;
  float brush_size_, brush_weight_;
  float transparency_;
  ci::Color color_picker_;
  std::string message_;
  std::queue<std::pair<cv::Vec2i, MOUSECLICK>> paint_queue_;
  std::vector<bool> selected_colors;
  std::list<int> selected_list;
  ci::Font font_;


};