using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1);
extern void write_to_flash();
extern void erase_all_timers();

void set_zerousers(void * pArg){
	arg *argument=(arg*)pArg;
	string algo;

	set_commonCmd(argument);

		erase_all_timers();

	algo="Zero users";
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
//	postLog(DLOGCLEAR,0);
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Set Zero\n");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
//	//	cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

