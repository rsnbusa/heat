
/*
 * set_statusSend.cpp

 *
 *  Created on: Apr 16, 2017
 *      Author: RSN
 */
using namespace std;
#include "setResets.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void delay(uint16_t a);
extern void write_to_flash();
extern void drawString(int x, int y, string que, int fsize, int align,displayType showit,overType erase);
extern void eraseMainScreen();
extern void postLog(int code,int code1,string que);

void set_reset(void * pArg){
	arg *argument=(arg*)pArg;
	string algo;

	set_commonCmd(argument);

	algo=getParameter(argument,"password");
	if(algo!="zipo")
	{
		algo="Not authorized";
		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),ERRORAUTH,false,false);            // send to someones browser when asked
		goto exit;
	}

	if(!displayf)
	{
		display.displayOn();
		displayf=true;
		if(aqui.disptime>0)
			xTimerStart(dispTimer,0);
	}

	if(xSemaphoreTake(I2CSem, portMAX_DELAY))
	{
		eraseMainScreen();
		drawString(64, 20, "RESET",24, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);
		xSemaphoreGive(I2CSem);
	}

	algo="Will reset in 5 seconds";

	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	if(aqui.traceflag & (1<<CMDD))
		printf("[CMDD]reset\n");                  // A general status condition for display. See routine for numbers.
	delay(5000);
	esp_restart();
	exit:
	algo="";
	//useless but....
//	if(argument->typeMsg)
//		cJSON_Delete(argument->theRoot);
//	free(pArg);
//	vTaskDelete(NULL);
}

void set_resetstats(void * pArg){
	arg *argument=(arg*)pArg;
	string algo;

	set_commonCmd(argument);

	algo=getParameter(argument,"password");
	if(algo!="zipo")
	{
		algo="Not authorized";
		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),ERRORAUTH,false,false);            // send to someones browser when asked
		goto exit;
	}
	write_to_flash();
	if(  xSemaphoreTake(logSem, portMAX_DELAY))
	{
		fclose(bitacora);
		bitacora = fopen("/spiflash/log.txt", "w"); //truncate
		if(bitacora)
		{
			fclose(bitacora); //Close and reopen r+
			if(aqui.traceflag & (1<<CMDD))
				printf("[CMDD]Log Cleared\n");
			xSemaphoreGive(logSem);
			bitacora = fopen("/spiflash/log.txt", "r+"); //truncate

		}
		else
			xSemaphoreGive(logSem);

	}
	algo="Reset stats";

	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	if(aqui.traceflag & (1<<CMDD))
		printf("[CMDD]ResetStats\n");                  // A general status condition for display. See routine for numbers.

	exit:
	algo="";
	postLog(DLOGCLEAR,0,"Stats reset");
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
////		cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}



