
/*
 * set_statusSend.cpp

 *
 *  Created on: Apr 16, 2017
 *      Author: RSN
 */
using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void delay(uint16_t a);
#define MAXLOG 840

void set_readlog(void * pArg){
	arg *argument=(arg*)pArg;
	char *buffer;
	string algo;

	if(argument->typeMsg!=1)
	{
		algo="Cmd only in mqtt format";
		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),NOERROR,false,false);            // send to someone's browser when asked
		goto exit;
	}
	set_commonCmd(argument);

	algo=getParameter(argument,"password");
	if(algo!="zipo")
	{
		algo="Not authorized";
		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),ERRORAUTH,false,false);            // send to someone's browser when asked
		goto exit;
	}

	//limit the size of the output to MAXHTTP
	fseek (bitacora , 0 , SEEK_END);
	int lSize;
	lSize=ftell (bitacora);
	rewind (bitacora);
	if(lSize>MAXHTTP){
		// advance the difference from Total - Max
		lSize-=MAXHTTP;
		fseek (bitacora , lSize , SEEK_SET); //move to that position
		lSize=MAXHTTP;
	}

	lSize+=4; //2 for code and 2 for centinel



//use spare buffer from ota. Malloc can not be later freed before mqtt ends message which is unknown time

	ota_write_data[0]='9'; //use this buffer
	ota_write_data[1]='9';
	memset(&ota_write_data[2],0xa0,2); //Centinel is A0A0

	fread (&ota_write_data[4],1,lSize-2,bitacora);

	if(uidStr != "") //If did not received a UID send without UID as a queue subheader
					spublishTopic=string(APP)+"/"+string(aqui.groupName)+"/"+string(aqui.heaterName)+"/"+uidStr+"/MSG";
				else
					spublishTopic=string(APP)+"/"+string(aqui.groupName)+"/"+string(aqui.heaterName)+"/MSG";
	if(aqui.traceflag & (1<<CMDD))
		printf("[CMDD]Readlog Queue %s sizemsg %d comm %p\n",spublishTopic.c_str(),lSize,argument->pComm);
	  esp_mqtt_client_publish(argument->pComm, (char*)spublishTopic.c_str(), ota_write_data,lSize, 0, 0);

	if(aqui.traceflag & (1<<CMDD))
		printf("[CMDD]readLog\n");

	exit:algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
////		cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}
