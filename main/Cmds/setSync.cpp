using namespace std;
#include "setClearLog.h"

extern void set_commonCmd(arg* pArg);
extern string getParameter(arg* argument,string cual);
extern void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
extern void postLog(int code,int code1,string text);
extern void write_to_flash();
extern void relay(u8 como);
extern void drawString(int x, int y, string que, int fsize, int align,displayType showit,overType erase);
extern void eraseMainScreen();

void set_sync(void * pArg){
	arg *argument=(arg*)pArg;
	string algo,state;
	int  posid;
    tickets lticket;
	esp_err_t q ;


	set_commonCmd(argument);

    state=getParameter(argument,"pos");
      if (state !="")
      {
          posid=atoi(state.c_str());
          state=getParameter(argument,"id");
          if (state !="")
          {
        	  memcpy(&lticket.userName,state.c_str(),state.length());
        	  lticket.userName[state.length()]=0;
        	  memcpy(&aqui.timerNames[posid],state.c_str(),state.length());
        	  aqui.timerNames[posid][state.length()]=0;

              state=getParameter(argument,"day");
              if (state!="")
              {
                  lticket.days=atoi(state.c_str()); //Bit for every day. Sunday-M-T-W-T-F-S
                  state=getParameter(argument,"fromdate");//hour
                  if (state !="")
                  {
                      lticket.fromDate=atoi(state.c_str());
                      state=getParameter(argument,"duration");
                      if (state !="")
                      {
                          lticket.duration=atoi(state.c_str());
                          state=getParameter(argument,"notis");
                          lticket.notifications=atoi(state.c_str());
                          state=getParameter(argument,"onOff");
                          lticket.onOff=atoi(state.c_str());
                          state=getParameter(argument,"temp");
                          lticket.requiredTemp=atoi(state.c_str());
                          lticket.minAmp=lticket.maxAmp=lticket.acumTime=lticket.avgAmps=lticket.acumAmps=0.0;
                          lticket.TimeStart=lticket.TimeStop=lticket.fired=lticket.starts=lticket.tempStart=lticket.tempStop=0;
                          lticket.ampCount=0;
                          aqui.ucount++;

                          q=nvs_set_blob(knvshandle,lticket.userName,(void*)&lticket,sizeof(tickets)); //write a timer ticket name as Passed
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
							 write_to_flash();
                      }
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
			drawString(64, 20, "SYNC",24, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);
//			drawString(70, 0, "              ", 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
			xSemaphoreGive(I2CSem);
		}
	exit:
	algo="Sync done";
	sendResponse( argument->pComm,argument->typeMsg, algo,algo.length(),MINFO,false,false);            // send to someones browser when asked
	postLog(TIMSYNC,0,"SyncTimer");//Sync
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Set OnOff\n");
	algo="";
//	if(argument->typeMsg){
//		cJSON_Delete(argument->theRoot);
////		cJSON_free(argument->theRoot);
//	}
//	free(pArg);
//	vTaskDelete(NULL);
}

