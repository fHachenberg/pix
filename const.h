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

Description: Contains several constants used in the algorithm and interface. They are
stored in this single, seperate file for ease of access. 
*/

#pragma once

#include "cinder/app/AppBasic.h"

namespace pix_research{

  //algorithm constants
  const float kDT = .7f;
  const float kTF = 1.0f;
  const float kSubclusterPertubation = .8f;
  const float kMaxUndo = 12.0f;
  const float kPaletteErrorTolerance = 1.0f;
  const float kSubclusterTolerance = 1.6f;
  const float kT0SafteyFactor = 1.1f;

  //UI constraints
  const float kBarX = 0.01f;
  const float kBarY = 0.05f;
  const float kBarW = 0.23f;
  const float kBarH = 0.90f;

  const float kPaletteX = 0.76f;
  const float kPaletteY = 0.51f;
  const float kPaletteW = 0.23f;
  const float kPaletteH = 0.44f;
  const int   kLabelH   = 16;
  const int   kTextShift = 3;

  const float kMainViewW = 0.5f;
  const float kMainViewH = 0.9f;
  const float kMainViewX = 0.25f;
  const float kMainViewY = 0.05f;

  const float kPreviewX = 0.76f;
  const float kPreviewY = .05;
  const float kPreviewW = .23f;
  const float kPreviewH = .44f;

  const ci::ColorA kBrightColor = ci::ColorA(0.0f,0.0f,0.0f,.3f);
  const ci::ColorA kDarkColor = ci::ColorA(1.0f,1.0f,1.0f,.3f);
  const ci::Color kBackgroundColor = ci::Color(.2f,.2f,.2f);
  const ci::Color kLineColor = ci::Color(.5f,.5f,.5f);
  const ci::Color kTitlebarColor = ci::Color(.1f,.1f,.1f);

  const std::string kFontStyle = "Tahoma";
  const int kFontSize = 18;

}