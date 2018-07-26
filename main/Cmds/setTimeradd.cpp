using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1,string que);
extern void write_to_flash();
extern void relay(u8 como);
extern void launchTimer(int a);
extern void drawString(int x, int y, string que, int fsize, int align,displayType showit,overType erase);
extern void eraseMainScreen();

void set_timeradd(void * pArg){
	arg *argument=(arg*)pArg;
	string algo,state;
	tickets *lticket=NULL;
	bool addf=false;

	set_commonCmd(argument);

//	algo=getParameter(argument,"password");
//	if(algo!="zipo")
//	{
//		algo="Not authorized";
//		sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),ERRORAUTH,false,false);            // send to someones browser when asked
//		goto exit;
//	}
	lticket=(tickets*) malloc(sizeof(tickets));

    state=getParameter(argument,"id");
            if (state !="")
            {
            	memcpy(lticket->userName,state.c_str(),state.length()+1);
            	lticket->userName[state.length()]=0;
            	memcpy(aqui.timerNames[aqui.ucount++],state.c_str(),state.length()+1); //save name in Config and increase tiemr count Ucount
                state=getParameter(argument,"day");
                if (state!="")
                {
                    lticket->days=atoi(state.c_str()); //Bit for every day. Sunday-M-T-W-T-F-S
                    state=getParameter(argument,"fromdate");//hour
                    if (state !="")
                    {
                        lticket->fromDate=atoi(state.c_str()); //the unix timestamp. ALL are with data 2000/1/1 and the hour required
                        state=getParameter(argument,"duration");
                        if (state !="")
                        {
                            lticket->starts=lticket->notifications=lticket->onOff=lticket->requiredTemp=lticket->ampCount=0;
                            lticket->minAmp=999.9;
                            lticket->maxAmp=lticket->acumAmps=lticket->avgAmps=lticket->nowamp=lticket->nowkwh=lticket->kwh=0.0;
                            lticket->acumTime=lticket->TimeStart=lticket->TimeStop=lticket->fired=lticket->tempStart=lticket->tempStop=0;

                            lticket->duration=atoi(state.c_str());
                            state=getParameter(argument,"temp");
                            if (state!="")
                                lticket->requiredTemp=atoi(state.c_str());
                            state=getParameter(argument,"notis");
                                lticket->notifications=atoi(state.c_str());
                            state=getParameter(argument,"onOff");
                                lticket->onOff=atoi(state.c_str());

                                //ready must add it
                                // if it exists it will be replaced

                            	esp_err_t q ;
                            	q=nvs_set_blob(knvshandle,lticket->userName,(void*)lticket,sizeof(tickets)); //write a timer ticket name as Passed

                            	if (q !=ESP_OK)
                            	{
                            		printf("Error write ticket %d\n",q);
                            		aqui.ucount--;
                            		goto exit;// do not update config
                            	}
                            	 q = nvs_commit(knvshandle);
                            	 if (q !=ESP_OK)
                            	 {
                            	 		printf("Commit Error Timeradd %d\n",q);
                                		aqui.ucount--;
                            	 		goto exit; // do not update config
                            	 }
                            	 addf=true;
                            	 write_to_flash();
                            	 printf("To launch from add %d\n",aqui.ucount-1);
                            	 launchTimer(aqui.ucount-1);
                            	 q = nvs_commit(nvshandle);
                            	 if (q !=ESP_OK)
                            	 		printf("Commit Error %x\n",q);
                        }
                    }
                }
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
    			drawString(64, 19, "T-ADD",24, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);
    			xSemaphoreGive(I2CSem);
    		}

	exit:
	free(lticket);
	algo=addf?"Timer added":"Timer not added";
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	postLog(TIMADD,0,"TimerAdd");
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Timer Add %s\n",addf?"Yes":"No");
	//useless but....
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
//	//	cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

