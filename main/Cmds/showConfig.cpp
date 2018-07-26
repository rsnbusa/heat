/*
 * readFlash.cpp
 *
 *  Created on: Apr 16, 2017
 *      Author: RSN
 */

#include "readFlash.h"
extern float get_temp(u8 cual);
extern void delay(uint16_t a);

string makeDateString(time_t t)
{
	char local[40];
	struct tm timeinfo;
	if (t==0)
		time(&t);
	localtime_r(&t, &timeinfo);
	sprintf(local,"%02d/%02d/%02d %02d:%02d:%02d",timeinfo.tm_mday,timeinfo.tm_mon+1,1900+timeinfo.tm_year,timeinfo.tm_hour,
			timeinfo.tm_min,timeinfo.tm_sec);
	return string(local);
}


void print_date_time(string que,time_t t)
{
	struct tm timeinfo;

	localtime_r(&t, &timeinfo);
	printf("[%s] %02d/%02d/%02d %02d:%02d:%02d\n",que.c_str(),timeinfo.tm_mday,timeinfo.tm_mon,1900+timeinfo.tm_year,timeinfo.tm_hour,
			timeinfo.tm_min,timeinfo.tm_sec);
}

void show_config(u8 meter, bool full) // read flash and if HOW display Status message for terminal
{
	char textl[100];
	esp_err_t q ;
	size_t largo;
	tickets *lticket;
	TickType_t xRemainingTime;
	struct tm timeinfo;
	time_t now = 0;
	string s1,s2,s3;


		time(&now);
		print_date_time(string("Flash Read HeatIoT "),now);
		if(full)
		{
			if(aqui.oneTankf)
				printf ("Last Compile %s-%s  Temp(Tank) %.2fC\n",__DATE__,__TIME__,get_temp(0) );
			else
				printf ("Last Compile %s-%s Temp(Amb) %.2fC Temp(Tank) %.2fC\n",__DATE__,__TIME__,get_temp(!aqui.lastTankId),get_temp(aqui.lastTankId) );

			if(aqui.centinel==CENTINEL)
				printf("Valid Centinel SNTP:%s\n",timef?"Y":"N");
			u32 diffd=now-aqui.lastTime;
			u16 horas=diffd/3600;
			u16 min=(diffd-(horas*3600))/60;
			u16 secs=diffd-(horas*3600)-(min*60);
			printf("[Last Boot: %s] [Elapsed %02d:%02d:%02d] [Previous Boot %s] [Count:%d ResetCode:0x%02x]\n",s1.c_str(),horas,min,secs,
					makeDateString(aqui.preLastTime).c_str(),aqui.bootcount,aqui.lastResetCode);
			for(int a=0;a<5;a++)
				if(aqui.ssid[a][0]!=0)
					printf("[SSID[%d]:%s-%s %s\n",a,aqui.ssid[a],aqui.pass[a],curSSID==a ?"*":" ");

			printf( "[IP:" IPSTR "] ", IP2STR(&localIp));

			u8 mac[6];
			esp_wifi_get_mac(WIFI_IF_STA, mac);
			sprintf(textl,"[MAC %2x%2x] ",mac[4],mac[5]);
			string mmac=string(textl);
			printf("%s",mmac.c_str());
			mmac="";
			printf("[AP Name:%s]\n",AP_NameString.c_str());
			printf("Heater Name:%s Working:%s\n",aqui.heaterName,aqui.working?"On":"Off");
			printf("MQTT Server:[%s:%d] Client: %s Connected:%s User:[%s] Passw:[%s] SSL %s TSChan:[%s] TSAPIk:[%s]\n",aqui.mqtt,aqui.mqttport,settings.client_id,mqttf?"Yes":"No",
					aqui.mqttUser,aqui.mqttPass,aqui.ssl?"Yes":"No",aqui.thingsChannel,aqui.thingsAPIkey);
			printf("Cmd Queue:%s\n",cmdTopic.c_str());
			printf("Answer Queue:%s\n",spublishTopic.c_str());
			printf("Update Server:%s\n",aqui.domain);
			nameStr=string(APP)+".bin";
			printf("[Version OTA-Updater %s] ",aqui.actualVersion);
			printf("[Firmware %s @ %s]\n",nameStr.c_str(),makeDateString(aqui.lastUpload).c_str());

			//          printf("Accepted Ids %d\n",aqui.ucount);
			//       print_log();
		}

		printf("\nSettings Volts %d Watts %d AutoTank %d ControlTemp %d TankId %d Sleep %d 1Tank %d Monitor %d OffsetADC %.1f MonTime %d\n\n",aqui.volts,aqui.watts,
								aqui.autoTank,aqui.controlTemp,aqui.lastTankId,aqui.disptime,aqui.oneTankf,aqui.monitorf,aqui.calib,aqui.monitorTime);
		if(aqui.traceflag>0)
			{
			printf("Trace Flags ");

						for (int a=0;a<NKEYS/2;a++)
							if (aqui.traceflag & (1<<a))
							{
								if(a<(NKEYS/2)-1)
									printf("%s-",lookuptable[a].key);
								else
									printf("%s",lookuptable[a].key);
							}
						printf("\n");
			}
		else
			printf("No trace flags\n");
		if(sonUid>0)
		{
			printf("Connected Users %d\n",sonUid);
			for (int a=0;a<sonUid;a++){
				printf("Uid %s ",montonUid[a].c_str());
				print_date_time(string("LogIn"),uidLogin[a] );
			}
		}

		if(aqui.ucount>0)
		{
			lticket=(tickets*)malloc(sizeof(tickets));

			largo=sizeof(tickets);
	//		printf("Timers %d 1ms=%d Tick\n",aqui.ucount,pdMS_TO_TICKS(1) );
			for (int a=0;a<10;a++){
				if(aqui.timerNames[a][0]!=0)
					{
				printf("Timer[%d] %s ",a,aqui.timerNames[a]);
				q=nvs_get_blob(knvshandle,aqui.timerNames[a],(void*)lticket,&largo);
				if (q!=ESP_OK)
					printf("Internal error timer %s\n",aqui.timerNames[a]);
				else
				{
					//time_t este=lticket->fromDate;
			char diass[8]="-------";
			char diaSemana[8]="SMTWTFS";
			//printf("Dias %x\n",lticket->days);
			for (int a=0;a<7;a++)
			{
				if(lticket->days & (1<<a))
					diass[a]=diaSemana[a];
			}
			diass[7]=0;
			string nada=string(diass);
					localtime_r((time_t*)&lticket->fromDate, &timeinfo);
					printf("From:%02d:%02d:%02d (%ld) Dur:%ld Days:%s OnOff:%d STemp %d RTemp %d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,lticket->fromDate,
							lticket->duration,nada.c_str(),lticket->onOff, lticket->tempStart, lticket->requiredTemp);
					printf("Starts:%d Tstart:%d (%s) TStop:%d (%s)\n",lticket->starts,lticket->tempStart,makeDateString(lticket->TimeStart).c_str(),
							lticket->tempStop,makeDateString(lticket->TimeStop).c_str());
					printf("Acum KwH:%.05f MinAmp: %.02f MaxAmp:%.02f AvgAmps:%.02f TotalTimeUp %d\n",lticket->kwh,lticket->minAmp,lticket->maxAmp,
							lticket->acumAmps/(double)lticket->ampCount, lticket->acumTime);
					if(startTimer[a])
					{
						printf("Start Timer active: %s End Timer Active:%s\n",xTimerIsTimerActive( startTimer[a] )?"Yes":"No",xTimerIsTimerActive( endTimer[a] )?"Yes":"No");
						if(xTimerIsTimerActive( startTimer[a] ))
						{
						xRemainingTime = xTimerGetExpiryTime( startTimer[a] ) - xTaskGetTickCount();
						 printf("Time remaining to Start:%d secs (%d mins) ",xRemainingTime/1000,xRemainingTime/60000);
						}
						if(xTimerIsTimerActive( endTimer[a] ))
						{
						 xRemainingTime = xTimerGetExpiryTime( endTimer[a] ) - xTaskGetTickCount();
						 printf("Time remaining to End:%d secs (%d mins)\n",xRemainingTime/1000,xRemainingTime/60000);
						}
					}
					else
						printf("Timers are not launched\n");
				}
			}
			}
			free(lticket);
		}
}




