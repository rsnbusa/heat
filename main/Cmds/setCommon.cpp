/*
 * setCommon.cpp
 *
 *  Created on: Apr 16, 2017
 *      Author: RSN
 */

#include "setCommon.h"
extern string getParameter(arg* argument,string cual);

void addUid(string cual)
{
	time_t now = 0;

	if (sonUid>4)
				return;
	for (int a=0;a<sonUid;a++)
	{
		if(strcmp(cual.c_str(),montonUid[a].c_str())==0)
			return ;
	}
		time(&now);
		montonUid[sonUid]=cual;
		uidLogin[sonUid]=now;
		sonUid++;
}

void set_commonCmd(arg* pArg)  //not really uselfull but was to use a single routine to check common parameters.
{
	uidStr=getParameter(pArg,"uid");
	addUid(uidStr);
}




