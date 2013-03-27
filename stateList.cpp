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
*/

#include "stateList.h"

stateList::stateList(int maxSize)
{
  head_ = new stateNode(new pixState());
  current_ = head_;
  max_size_ = maxSize;
  current_size_ = 1;
}

void stateList::push_copy()
{
  stateNode * next = new stateNode(new pixState(*(current_->val)));

  stateNode * delme = current_->next;
  while(delme != NULL)
  {
    stateNode * delnext = delme->next;
    delete delme;
    delme = delnext;
    current_size_--;
  }

  current_->next = next;
  next->prev = current_;
  current_ = next;
  current_size_++;

  while(current_size_ > max_size_)
  {
    head_ = head_->next;
    delete head_->prev;
    head_->prev = NULL;
    current_size_--;
  }

}

void stateList::stepBack()
{
  if(current_->prev != NULL) current_ = current_->prev;
}
void stateList::stepForward()
{
  if(current_->next != NULL) current_= current_->next;
}