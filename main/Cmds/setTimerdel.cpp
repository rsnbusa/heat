using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1,string que);
extern void write_to_flash();
extern void relay(u8 como);
extern void drawString(int x, int y, string que, int fsize, int align,displayType showit,overType erase);
extern void eraseMainScreen();

void set_timerdel(void * pArg){
	arg *argument=(arg*)pArg;
	string algo;
	esp_err_t q ;
	char textl[60];


	set_commonCmd(argument);
	tickets *lticket=(tickets*)malloc(sizeof(tickets));

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
	        	size_t largo=sizeof(tickets);
				q=nvs_get_blob(knvshandle,algo.c_str(),(void*)lticket,&largo);
				if(q==ESP_OK && globalTimer)
				{
					if (strcmp(lticket->userName,globalTimer->userName)==0)
					{
						xTimerStop(endTimer[globalTimer->internalNumber],0);
						relay(OFF);
						//also kill the TankManager that is active and the Status global activity
						vTaskDelete(tankHandle);
						globalTimer=NULL;
					}
				}

	        	q=nvs_erase_key(knvshandle,algo.c_str());
	        	if(q!=ESP_OK)
	        	{
	        		sprintf(textl,"Error erasing %s not found",algo.c_str());
	        		algo=string(textl);
	        		printf("%s\n",textl);
	        		goto exit;
	        	}
	        	//now find pos in timersName and erase entry-stop/delete timers
	        	int pos=-1;

	        	for(int a=0;a<10;a++)
	        	{
	        		if (strcmp(aqui.timerNames[a],algo.c_str())==0)
	        		{
	        			pos=a;
	        			break;
	        		}
	        	}
	        	if (pos==-1)
	        	{
	        		sprintf(textl,"internal error TimerNames not found");
	        		algo=string(textl);
	        		printf("%s\n",textl);
	        		goto exit;
	        	}

	        	//Stop and delete Start and End timers
//	        	if(startTimer[pos]){ //sanity check else crash
//	        		xTimerStop(startTimer[pos],0);
//	        		xTimerDelete(startTimer[pos],0);
//	        	}
//	        	if(endTimer[pos]){//sanity check else crash
//	        		xTimerStop(endTimer[pos],0);
//	        		xTimerDelete(endTimer[pos],0);
//	        	}
	       	 q = nvs_commit(knvshandle);
	       	 if (q !=ESP_OK)
	       	 		printf("Commit ErrorDel %d\n",q);
        		aqui.ucount--; //one less timer count for AddTimer to work properly

	        	// move arrays up timerNames, start-end timers
	        	if (pos<(MAXTIMERS-1))
	        	{
	        		memmove(&aqui.timerNames[pos],&aqui.timerNames[pos+1],(MAXTIMERS-pos)*MAXCHARS);
	        		memmove(&startTimer[pos],&startTimer[pos+1],(MAXTIMERS-pos)*sizeof(TimerHandle_t));
	        		memmove(&endTimer[pos],&endTimer[pos+1],(MAXTIMERS-pos)*sizeof(TimerHandle_t));
	        	}
	        	// zero the bottom of all arrays
        		memset(&aqui.timerNames[MAXTIMERS-1],0,MAXCHARS);
        		startTimer[MAXTIMERS-1]=0;
    	        endTimer[MAXTIMERS-1]=0;

	        }
	        write_to_flash();
	        algo="Timer deleted";

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
				drawString(64, 19, "T-DEL",24, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);
				xSemaphoreGive(I2CSem);
			}

	exit:
	free(lticket);
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Delete Timer\n");
	postLog(TIMDEL,0,"TimerDel");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
////		cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

