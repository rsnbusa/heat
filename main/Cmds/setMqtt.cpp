using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1);
extern void write_to_flash();
extern void relay(u8 como);

void set_mqtt(void * pArg){
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

	algo=getParameter(argument,"qqqq");
	        if (algo !="")
	        {
				 memcpy(aqui.mqtt,algo.c_str(),algo.length());
				 aqui.mqtt[algo.length()]=0;

				 algo=getParameter(argument,"port");
					if (algo !="")
					aqui.mqttport=atoi(algo.c_str());

				 algo=getParameter(argument,"uupp");
					if (algo !="")
					{
						 memcpy(aqui.mqttUser,algo.c_str(),algo.length());
						 aqui.mqttUser[algo.length()]=0;
					}
				 algo=getParameter(argument,"passq");
					if (algo !="")
					{
						 memcpy(aqui.mqttPass,algo.c_str(),algo.length());
						 aqui.mqttPass[algo.length()]=0;
					}
			        write_to_flash();
			        algo="MqttSet";
	        }
	        else
	        	algo="No ParamsMqtt";

	exit:
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Set MQTT\n");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
//		cJSON_free(argument->theRoot);
//	}
////	free(pArg);
//	vTaskDelete(NULL);
}

