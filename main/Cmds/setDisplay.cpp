using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1, string que);
extern void write_to_flash();

void set_display(void * pArg){
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

	algo=getParameter(argument,"disptime");
	        if (algo !="")
	        {
	            aqui.disptime=atoi(algo.c_str());
		        write_to_flash();
	        }

	exit:
	algo="DisplayTime";
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	postLog(LINTERNAL,0,"Display Set");
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Set Display\n");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
////		cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

