/*
 * eraseMeter.cpp
 *
 *  Created on: Apr 18, 2017
 *      Author: RSN
 */

#include "eraseMeter.h"
extern void write_to_flash();
extern void postLog(int code, string mensaje);

void erase_config() //do the dirty work
{
	memset(&aqui,0,sizeof(aqui));
	aqui.centinel=CENTINEL;
	memcpy(aqui.mqtt,MQTTSERVER,sizeof(MQTTSERVER));//fixed mosquito server
	aqui.mqtt[sizeof(MQTTSERVER)]=0;
	printf("Mqtt Erase %s\n",aqui.mqtt);
	memcpy(aqui.domain,"feediot.co.nf",13);//fixed mosquito server feediot.co.nf
	aqui.domain[13]=0;
	aqui.calib=100.0;
	aqui.disptime=1; //one minute
	write_to_flash();

	printf("Centinel %x\n",aqui.centinel);
}



