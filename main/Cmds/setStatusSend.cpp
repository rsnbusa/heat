using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1);
extern void write_to_flash();
extern void relay(u8 como);
extern void delay(u16 cuanto);
extern float get_temp(u8 cual);

void set_statusSend(void * pArg){
	arg *argument=(arg*)pArg;
	string algo;
	char textl[120],textamp[12],textkwhnow[12],textkwhacum[12];
	float tTemp=0;

	set_commonCmd(argument);

//	algo=getParameter(argument,"password");
//	if(algo!="zipo")
//	{
//		algo="Not authorized";
//		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),ERRORAUTH,false,false);            // send to someones browser when asked
//		goto exit;
//	}
//	if(!aqui.oneTankf)
//		tTemp=get_temp(aqui.lastTankId);
//	else
//		tTemp=get_temp(0);
		textamp[0]=0;
		textkwhnow[0]=0;
		textkwhacum[0]=0;

		if(globalTimer)
		{
			sprintf(textamp,"%.02f",globalTimer->nowamp);
			sprintf(textkwhnow,"%.5f",globalTimer->nowkwh);
			sprintf(textkwhacum,"%.5f",globalTimer->kwh);
		}

		if(!aqui.oneTankf)
			sprintf(textl,"%d:%d:%d:%d:%d:%d:%s:%s:%s",(int)get_temp(!aqui.lastTankId),0,(int)get_temp(aqui.lastTankId),0,aqui.working,globalTimer?1:0,
					textkwhnow,textkwhacum,textamp); // ambient temp and humidity, water temp, water outflowing,server status and state machine
	//	sprintf(textl,"%d:%d:%d:%d:%d:%d:%s:%s:%s",1,0,2,0,aqui.working,globalTimer?1:0,textkwhnow,textkwhacum,textamp); // ambient temp and humidity, water temp, water outflowing,server status and state machine
		else
			sprintf(textl,"%d:%d:%d:%d:%d:%d:%s:%s:%s",0,0,(int)tTemp,0,aqui.working,globalTimer?1:0,textkwhnow,textkwhacum,textamp); // ambient temp and humidity, water temp, water outflowing,server status and state machine




		algo=string(textl);
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Set Status\n");
	if(displayf)
	{
	if(xSemaphoreTake(I2CSem, portMAX_DELAY))
		{
				display.setColor(WHITE);
				display.fillCircle(STATUSX, STATUSY, 5);
				display.display();
				delay(300);
				display.setColor(BLACK);
				display.fillCircle(STATUSX, STATUSY, 5);
				display.display();
			xSemaphoreGive(I2CSem);
		}
	}
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
//	//	cJSON_free(argument->theRoot);
//	}

//	free(pArg);
//	vTaskDelete(NULL);
}

