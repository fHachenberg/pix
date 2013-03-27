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
	mHead = new stateNode(new pixState());
	mCurr = mHead;
	mMaxSize = maxSize;
	mCurSize = 1;
}

void stateList::push_copy()
{
	stateNode * next = new stateNode(new pixState(*(mCurr->val)));
	
	stateNode * delme = mCurr->next;
	while(delme != NULL)
	{
		stateNode * delnext = delme->next;
		delete delme;
		delme = delnext;
		mCurSize--;
	}

	mCurr->next = next;
	next->prev = mCurr;
	mCurr = next;
	mCurSize++;

	while(mCurSize > mMaxSize)
	{
		mHead = mHead->next;
		delete mHead->prev;
		mHead->prev = NULL;
		mCurSize--;
	}

}

void stateList::stepBack()
{
	if(mCurr->prev != NULL) mCurr = mCurr->prev;
}
void stateList::stepForward()
{
	if(mCurr->next != NULL) mCurr= mCurr->next;
}