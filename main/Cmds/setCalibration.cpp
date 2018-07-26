using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void write_to_flash();
extern void drawString(int x, int y, string que, int fsize, int align,displayType showit,overType erase);
extern void eraseMainScreen();
extern void postLog(int code,int code1,string que);


void set_calibration(void * pArg){
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

	algo=getParameter(argument,"calib");
	        if (algo !="")
	        {
	            aqui.calib =atof(algo.c_str());
		        write_to_flash();
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
				drawString(64, 20, "AMPCAL",24, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);
				xSemaphoreGive(I2CSem);
			}

	exit:
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	postLog(LINTERNAL,0,"AMP Cal");
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Set Calib\n");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
//	//	cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

