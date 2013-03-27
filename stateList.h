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

Description: Used to store the states of the algorithm for undo/redo operations. 
This is techinically not necessary for the algorithm to run, but greately reduces
the overhead of undo/redo operations with the accompanying UI.
*/

#pragma once

#include "opencv2/opencv.hpp"
#include <vector>
#include <list>

enum STATE;

struct pixState
{
  cv::Mat superpixel_pos;
  cv::Mat superpixel_color;
  cv::Mat palette_assign;
  std::vector<cv::Vec3f> palette;
  std::vector<float> prob_c;
  std::vector<bool> locked_colors;
  std::vector<std::list<int> > pixel_constraints;
  std::vector<std::pair<int,int> > sub_superpixel_pairs;
  int iteration;
  float saturation;
  pixState(){};

  pixState(const pixState& other)
  {
    other.superpixel_pos.copyTo(superpixel_pos);
    other.superpixel_color.copyTo(superpixel_color);
    superpixel_color = cv::Mat(other.superpixel_color);
    palette_assign = cv::Mat(other.palette_assign);
    palette = std::vector<cv::Vec3f>(other.palette);
    prob_c = std::vector<float>(other.prob_c);
    locked_colors = std::vector<bool>(other.locked_colors);
    pixel_constraints = std::vector<std::list<int>>(other.pixel_constraints);
    sub_superpixel_pairs = std::vector<std::pair<int,int>>(other.sub_superpixel_pairs);
    iteration = other.iteration;
    saturation = other.saturation;
  }
};




class stateList
{
  struct stateNode
  {
    pixState * val;
    stateNode * prev;
    stateNode * next;
    stateNode(pixState * n): val(n), prev(NULL), next(NULL){}
    ~stateNode(){ delete val;}
  };

 public: 

  //constructs a statelist with a single state
  stateList(int maxSize);
  ~stateList(){}
  //pushes a copy of the current state onto the list and makes it the current
  //state If the current state had any children, they are deleted. If the 
  //total number of states exceeds the maximum specified at construction, 
  //states are removed from the front of the list until this bound is met. 
  void push_copy();
  //moves the current state to the previous state, while retaining
  //the current state in memory. Does not change the state if no previous
  //state exists.
  void stepBack();
  //moves the current state to the next state, while retaining the
  //current state in memory. Does not change the state if no next state exists.
  void stepForward();
  //returns the current state
  pixState * getCur() {return current_->val;}

 private:
  stateNode* head_;
  stateNode* current_;
  int max_size_;
  int current_size_;

};
