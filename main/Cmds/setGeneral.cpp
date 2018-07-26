
/*
 * setGeneral.cpp

 *
 *  Created on: Apr 16, 2017
 *      Author: RSN
 */
using namespace std;
#include "setGeneral.h"
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void delay(uint16_t a);
extern void postLog(int code,int code1,string que);

void set_generalap(void * pArg){
	arg *argument=(arg*)pArg; //set pet name and put name in bonjour list if changed
//	char textl[40];
	string s1,webString,state;
	int res=0;

	webString="General Info set";
	set_commonCmd(argument);
	s1=getParameter(argument,"heater");
	if (s1 !="")
	{
		memcpy((void*)aqui.heaterName,s1.c_str(),s1.length()+1);
        memcpy(aqui.groupName,aqui.heaterName,sizeof(aqui.heaterName));

		if(aqui.traceflag & (1<<CMDD))
			printf("[CMDD]Name %s\n",aqui.heaterName);

			state=getParameter(argument,"autoTemp");
			if(state!="")
				aqui.controlTemp=atoi(state.c_str());

	        state=getParameter(argument,"watts");
			if(state!="")
				aqui.watts=atoi(state.c_str());

	        state=getParameter(argument,"volts");
			if(state!="")
				aqui.volts=atoi(state.c_str());

			state=getParameter(argument,"tschan");
			if(state!="")
				memcpy(aqui.thingsChannel,state.c_str(),state.length());

	        state=getParameter(argument,"tskey");
			if(state!="")
				memcpy(aqui.thingsAPIkey,state.c_str(),state.length());

	        state=getParameter(argument,"kwh");
			if(state!="")
				aqui.kwh=atof(state.c_str());

	        state=getParameter(argument,"autot");
			if(state!="")
				aqui.autoTank=atoi(state.c_str());

	        state=getParameter(argument,"onet");
			if(state!="")
				aqui.oneTankf=atoi(state.c_str());

	        state=getParameter(argument,"monitor");
			if(state!="")
				aqui.monitorf=atoi(state.c_str());

	        state=getParameter(argument,"monTime");
			if(state!="")
				aqui.monitorTime=atoi(state.c_str());

	        s1=getParameter(argument,"reset");
	        if(s1!="")
	        	 res=atoi(s1.c_str());

	    aqui.calib=100.0;
		s1=getParameter(argument,"index");
		int index=atoi(s1.c_str());
		if (index>4)
			index=4;
		s1=getParameter(argument,"ap");

		// Set MQTT Server and Port
			state=getParameter(argument,"mqtts");
			if(state!="")
			{
				if(strcmp(aqui.mqtt,state.c_str())!=0)
				{
					res=true; //must restart change of mqtt server or new
					memcpy(&aqui.mqtt,state.c_str(),state.length()+1);
				}
				res=true;
				if(aqui.traceflag & (1<<CMDD))
					printf("Mqtt %s\n",aqui.mqtt);
				state=getParameter(argument,"mqttport");
				if(state!="")
					aqui.mqttport=atoi(state.c_str());
				else
					aqui.mqttport=1883; //default
				state=getParameter(argument,"ssl");
				if(aqui.ssl!=atoi(state.c_str()))
						res=true;
				if(state!="")
					aqui.ssl=atoi(state.c_str());
				else
					aqui.ssl=0; //default
				webString+="Q-";
			}
			state="";

		state=getParameter(argument,"mqttu");
		if(state!="")
		{
			memcpy(&aqui.mqttUser,state.c_str(),state.length()+1);
			state=getParameter(argument,"mqttp");
			if(aqui.traceflag & (1<<CMDD))
				printf("Mqtt password %s\n",state.c_str());
			if(state!="")
			{
				memcpy(&aqui.mqttPass,state.c_str(),state.length()+1);
				res=true; //restart
			}
			webString+="QU-";
		}
		state="";

		if (s1=="")
		{// it the update cmd not a AP option
			write_to_flash();
			webString="General Erased SSID";
			sendResponse( argument->pComm,argument->typeMsg, webString,webString.length(),MINFO,false,false);            // send to someones browser when asked
			goto exit;
		}
		memcpy(aqui.ssid[index],s1.c_str(),s1.length()+1);
		s1=getParameter(argument,"pass");
			memcpy(aqui.pass[index],s1.c_str(),s1.length()+1);


		sendResponse( argument->pComm,argument->typeMsg, webString,webString.length(),MINFO,false,false);
	//	delay(10000);


	}
exit:
write_to_flash();
if(res)
{
	int son=10;
	while (son--) // for some reason it does mongoose task is not polling. Force it to send
		mg_mgr_poll(&mgr, 10);
	printf("Restart\n");
//	delay(10000);
	esp_restart();
}
postLog(LINTERNAL,0,"Setup");
	s1=webString="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
//	//	cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}





