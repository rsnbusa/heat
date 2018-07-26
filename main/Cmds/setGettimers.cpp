using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1);
extern void write_to_flash();
extern void relay(u8 como);

void set_gettimers(void * pArg){
	arg *argument=(arg*)pArg;
	string algo;

	set_commonCmd(argument);

	  algo="";
	        for (int a=0;a<aqui.ucount;a++)
	        {
	       //     algo+=String(aqui.users[a].userName)+"|"+makeHourString(aqui.users[a].fromDate)+"|"+String(aqui.users[a].duration)+"|"+String(aqui.users[a].days)+"|"+String(aqui.users[a].notifications)
	       //     +"|"+String(aqui.users[a].onOff)+"|"+String(aqui.users[a].requiredTemp);
	            if(a<aqui.ucount-1) algo+="@";
	        }
	        if(tracef)
	            printf("GetTimers\n");


//	algo=getParameter(argument,"password");
//	if(algo!="zipo")
//	{
//		algo="Not authorized";
//		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),ERRORAUTH,false,false);            // send to someones browser when asked
//		goto exit;
//	}

;

	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
//	postLog(DLOGCLEAR,0);
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]get Timers\n");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
////		cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

