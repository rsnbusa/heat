/*
 * displayMananger.cpp
 *
 *  Created on: Apr 18, 2017
 *      Author: RSN
 */

#include "displayManager.h"
using namespace std;

extern uint32_t IRAM_ATTR millis();
extern float get_temp(u8 cual);
extern void delay(uint16_t a);
extern void postLog(int code, int code1, string que);
extern void eraseMainScreen();

void drawString(int x, int y, string que, int fsize, int align,displayType showit,overType erase)
{
	if(!displayf)
		return;

	if(fsize!=lastFont)
	{
		lastFont=fsize;
		switch (fsize)
		{
		case 10:
			display.setFont(ArialMT_Plain_10);
			break;
		case 16:
			display.setFont(ArialMT_Plain_16);
			break;
		case 24:
			display.setFont(ArialMT_Plain_24);
			break;
		default:
			break;
		}
	}

	if(lastalign!=align)
	{
		lastalign=align;

		switch (align) {
		case TEXT_ALIGN_LEFT:
			display.setTextAlignment(TEXT_ALIGN_LEFT);
			break;
		case TEXT_ALIGN_CENTER:
			display.setTextAlignment(TEXT_ALIGN_CENTER);
			break;
		case TEXT_ALIGN_RIGHT:
			display.setTextAlignment(TEXT_ALIGN_RIGHT);
			break;
		default:
			break;
		}
	}

	if(erase==REPLACE)
	{
		int w=display.getStringWidth((char*)que.c_str());
		int xx=0;
		switch (lastalign) {
		case TEXT_ALIGN_LEFT:
			xx=x;
			break;
		case TEXT_ALIGN_CENTER:
			xx=x-w/2;
			break;
		case TEXT_ALIGN_RIGHT:
			xx=x-w;
		default:
			break;
		}
		display.setColor(BLACK);
		display.fillRect(xx,y,w,lastFont+3);
		display.setColor(WHITE);
	}

	display.drawString(x,y,(char*)que.c_str());
	if (showit==DISPLAYIT)
		display.display();
}

void drawBars()
{
	//return;
	wifi_ap_record_t wifidata;
	if (esp_wifi_sta_get_ap_info(&wifidata)==0){
		//		printf("RSSI %d\n",wifidata.rssi);
		RSSI=80+wifidata.rssi;
	}
	for (int a=0;a<3;a++)
	{
		if (RSSI>RSSIVAL)
			display.fillRect(barX[a],YB-barH[a],WB,barH[a]);
		else
			display.drawRect(barX[a],YB-barH[a],WB,barH[a]);
		RSSI -= RSSIVAL;
	}
	if (mqttf)
	{
		if(xSemaphoreTake(I2CSem, portMAX_DELAY))
			{
				drawString(16, 5, string("m"), 10, TEXT_ALIGN_LEFT,DISPLAYIT, NOREP);
				xSemaphoreGive(I2CSem);
			}
	}
}


void setLogo(string cual)
{
//return;
	display.setColor(BLACK);
	display.clear();
	display.setColor(WHITE);
	drawString(64, 20, cual.c_str(),24, TEXT_ALIGN_CENTER,NODISPLAY, NOREP);
	drawBars();
	display.drawLine(0,18,127,18);
	display.drawLine(0,50,127,50);
//	display.drawLine(0,18,0,50);
//	display.drawLine(127,18,127,50);
//	display.display();
	oldtemp=0.0;
}

int findNextTimer()
{
	struct tm timeinfo = {  };
	time_t now;
	esp_err_t q;

	tickets *lticket=(tickets*)malloc(sizeof(tickets));
	size_t largo=sizeof(tickets);

	time(&now);
	localtime_r(&now, &timeinfo); // convert to struct to change date to 2000/1/1

	timeinfo.tm_mday=1;
	timeinfo.tm_mon=0;
	timeinfo.tm_year=100;
	// set to 1/1/2000 x:y:z
	now = mktime(&timeinfo); // now with same date so only time counts
	int cualf=-1;
	uint32_t mintime=0xffffffff;

//	if(aqui.ucount>0)
//		{

	//		printf("Timers %d 1ms=%d Tick\n",aqui.ucount,pdMS_TO_TICKS(1) );
			for (int a=0;a<aqui.ucount;a++){
				if(aqui.timerNames[a][0]!=0)
					{
					//	printf("Timer[%d] %s ",a,aqui.timerNames[a]);
						q=nvs_get_blob(knvshandle,aqui.timerNames[a],(void*)lticket,&largo);
						if (q!=ESP_OK){
							printf("Internal error timer %s\n",aqui.timerNames[a]);
							break;
						}

						if(lticket->fromDate>now)
							if(lticket->fromDate-now<mintime){
								cualf=a;
								mintime=lticket->fromDate-now;
							}
					}
			}
			free(lticket);
			return cualf;
	//	}
}

void showNextTimer( TimerHandle_t xTimer )
{
	string local;
	bool oldf;

	if(xTimer==nextTimer)
	{
		oldf=displayf;
		displayf=true;
		display.displayOn();

		if(!tankHandle)
		{
			if(xSemaphoreTake(I2CSem, portMAX_DELAY))
			{
				local=string((char*)pvTimerGetTimerID(xTimer));
				eraseMainScreen();
				drawString(64, 20,local,24, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);
				xSemaphoreGive(I2CSem);
			}
			if(aqui.traceflag & (1<<GEND))
				printf("[GEND]Next timer %s\n",local.c_str());
		}
		delay(10000);
		display.displayOff();
		displayf=oldf;
	}
}


void timerManager(void *arg) {
	time_t t = 0;
	struct tm timeinfo ;
	char textd[20],textt[20];
	u32 nheap;

	while(true)
	{
		nheap=xPortGetFreeHeapSize();

		if(aqui.traceflag & (1<<HEAPD))
			printf("[HEAPD]Heap %d\n",nheap);

		vTaskDelay(1000/portTICK_PERIOD_MS);
		time(&t);
		localtime_r(&t, &timeinfo);

		if (displayf)
		{
			if(xSemaphoreTake(I2CSem, portMAX_DELAY))
			{
				sprintf(textd,"%02d/%02d/%04d",timeinfo.tm_mday,timeinfo.tm_mon+1,1900+timeinfo.tm_year);
				sprintf(textt,"%02d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
				drawString(16, 5, mqttf?string("m"):string("   "), 10, TEXT_ALIGN_LEFT,NODISPLAY, REPLACE);
				drawString(0, 51, string(textd), 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
				drawString(86, 51, string(textt), 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
				drawString(61, 51, aqui.working?"On  ":"Off", 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
				xSemaphoreGive(I2CSem);
			}
		}

		// check for day change, and restart timers.
		if(timef && connf)
		{
	//		printf("change day %d vs todate %d\n",timeinfo.tm_yday,todate);

			if(timeinfo.tm_yday>todate)
			{
				postLog(14,todate,"ReloadTimers");
				printf("change day %d vs todate %d Reboot\n",timeinfo.tm_yday,todate);
				todate=timeinfo.tm_yday;

				vTaskDelay(1000/portTICK_PERIOD_MS);//wait 1 seconds
				esp_restart();//reboot is easier....
			}
		}

	}
}


void displayManager(void *arg) {
	//   gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
	//	int level = 0;
	char textl[20];
	string local;
	uint32_t tempwhen=0;
	float temp,diff;
	//oldState=stateVM;
	oldtemp=0.0;
	xTaskCreate(&timerManager,"timeMgr",4096,NULL, MGOS_TASK_PRIORITY, NULL);
	int cualTimer,oldTimer;
	nextTimer=xTimerCreate("nextTimer",60000/portTICK_PERIOD_MS,true,( void * ) 0,&showNextTimer);

	oldTimer=-1;
	u32 checkTimer=millis();

	while (true)
	{
		if(displayf)
		{
			if(millis()-tempwhen>1000 && numsensors>0)
			{
				temp=get_temp(!aqui.lastTankId);
				diff=temp-oldtemp;
				if(diff<0.0)
					diff*=-1.0;
				if (diff>0.3 && temp<130.0)
					{
					if(xSemaphoreTake(I2CSem, portMAX_DELAY))
						{
							sprintf(textl,"%.01fC\n",temp);
							drawString(64, 0, "     ", 10, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE); //50 middle
							drawString(64, 0, string(textl), 10, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);
							oldtemp=temp;
							xSemaphoreGive(I2CSem);
						}
					tempwhen=millis();
					}
			}
		}
		else
			if(millis()-checkTimer>60000 )
			{
				checkTimer=millis();
				cualTimer=findNextTimer();
				if (oldTimer!=cualTimer)
				{
					if(oldTimer==-1)
						xTimerStart(nextTimer,0); // Timer will semaphore a task waiting to display if No timer is Active (tankHandle is null)
					oldTimer=cualTimer;
				//	printf("Set timer name %s\n",aqui.timerNames[cualTimer]);
					vTimerSetTimerID(nextTimer,(void*)&aqui.timerNames[cualTimer]);
				}
			}


		vTaskDelay(100/portTICK_PERIOD_MS);
	}
}

