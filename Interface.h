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

Description: Adds additional functionality to cinder's adaptation of 
anttweakbar. Specifically, methods to set the position and size.
*/



#include "cinder\params\Params.h"
#include "cinder\Color.h"
#include <sstream>
#include "AntTweakBar.h"

using namespace cinder::params;
using namespace cinder;

class interface : public InterfaceGl
{
 public:
  interface(){};
  interface (const std::string &title, const Vec2i &size, const ColorA colorA = ColorA(.3f,.3f,.3f,.4f)) : InterfaceGl(title, size, colorA){};
  inline void setPosition(int x, int y)
  {
    std::stringstream ss;
    ss << "`" << (std::string)TwGetBarName(mBar.get()) << "`";
    ss << " position=";
    ss << "`" << x << " " << y << "`";
    TwDefine(ss.str().c_str());
  }
  inline void setSize(int w, int h)
  {
    std::stringstream ss;
    ss << "`" << (std::string)TwGetBarName(mBar.get()) << "`";
    ss << " size=";
    ss << "`" << w << " " << h << "`";
    TwDefine(ss.str().c_str());
  }
};
