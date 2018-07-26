using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1);
extern void write_to_flash();
extern void relay(u8 como);

void set_timeronoff(void * pArg){
	arg *argument=(arg*)pArg;
	string algo;
	int pos=-1;
	u8 ss=0;
	tickets lticket;

	set_commonCmd(argument);

//	algo=getParameter(argument,"password");
//	if(algo!="zipo")
//	{
//		algo="Not authorized";
//		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),ERRORAUTH,false,false);            // send to someones browser when asked
//		goto exit;
//	}

	algo=getParameter(argument,"name");
	        if (algo !="")
	        {
	        	esp_err_t q ;
	        	size_t largo=sizeof(tickets);
	        	q=nvs_get_blob(knvshandle,algo.c_str(),(void*)&lticket,&largo);
	        	if(q!=ESP_OK)
	        	{
	        		printf("Error reading timer TimerOnOff %s",algo.c_str());
	        		goto exit;
	        	}
	        	for (int a=0;a<10;a++)
	        	{
	        		if(strcmp(algo.c_str(),aqui.timerNames[a])==0)
	        		{
	        			pos=a;
	        			break;
	        		}
	        	}
	        	if(pos<0)
	        	{
	        		printf("Error internal find name %s",algo.c_str());
	        				goto exit;
	        	}

	        	algo=getParameter(argument,"st");
	        		        if (algo !="")
	        		        	ss=atoi(algo.c_str());

	        		    lticket.onOff=ss;

	        		        q=nvs_set_blob(knvshandle,lticket.userName,(void*)&lticket,sizeof(tickets)); //write a timer ticket name as Passed
	        		        if (q !=ESP_OK)
	        		             printf("Error Start rewriting timer onoff %s\n",lticket.userName);
	        		   	 q = nvs_commit(knvshandle);
	        		   		 if (q !=ESP_OK)
	        		   		 		printf("Commit Error %d\n",q);

	        }
	        write_to_flash();
exit:
	algo="Set timerOnOff";
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
//	postLog(DLOGCLEAR,0);
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Set TimerOnOff\n");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
//	//	cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

