#include <stdio.h>
#include <stdint.h>
#include "defines.h"
#include "forward.h"
#include "includes.h"
#include "projTypes.h"
#include "string.h"
#include "globals.h"
#include "cmds.h"


extern void postLog(int code,int code1,string que);
void processCmds(void * nc,cJSON * comands);
void sendResponse(void* comm,int msgTipo,string que,int len,int errorcode,bool withHeaders, bool retain);
void loadTimers();

using namespace std;

uart_port_t uart_num = UART_NUM_0;


void eraseMainScreen()
{
	display.setColor((OLEDDISPLAY_COLOR)0);
	display.fillRect(0,19,127,29);
	display.setColor((OLEDDISPLAY_COLOR)1);
}

uint32_t IRAM_ATTR millis()
{
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void delay(uint16_t a)
{
	vTaskDelay(a /  portTICK_RATE_MS);
}

void timerCallback( TimerHandle_t xTimer )
{

	if(xTimer==dispTimer)
	{
		display.displayOff();
		displayf=false;
		if(aqui.traceflag & (1<<GEND))
			printf("[GEND]Timer DISPLAY Expired\n");
	}
}


void mqttmanager(void * parg)
{
	mqttMsg mensa;

	mqttQ = xQueueCreate( 20, sizeof( mensa ) );
	if(mqttQ)
	{
		while(1)
		{
			if( xQueueReceive( mqttQ, &mensa, portMAX_DELAY ))
			{
			//	printf(" Heap MqttMgrIn %d need %d\n",xPortGetFreeHeapSize(),strlen(mensa.mensaje));

				if(aqui.traceflag & (1<<MQTTD))
					printf("[MQTTD]MqttMsg:%s\n",mensa.mensaje);
				root=cJSON_Parse( mensa.mensaje);
			//	printf(" Heap MqttMgrRoot %d\n",xPortGetFreeHeapSize());

				if(root==NULL)
					printf("Not valid Json\n");
				else
					processCmds(mensa.nc,root);
			//	printf(" Heap MqttMgr After %d\n",xPortGetFreeHeapSize());

			}
			delay(100);//just in case it fails to wait forever do no eat the cpu
		}
	}
	else
		delay(1000);
	vTaskDelete(NULL); //in case it reaches end
}

void write_to_flash() //save our configuration
{ //open close strategy for flash errors unexplainded lost of config
	esp_err_t q;
	q = nvs_open("config", NVS_READWRITE, &nvshandle);
	if(q!=ESP_OK){
		printf("Error opening NVS File\n");
	}
	delay(300); //some delay same problem.

	q=nvs_set_blob(nvshandle,"config",(void*)&aqui,sizeof(aqui));
	if (q ==ESP_OK)
		q = nvs_commit(nvshandle);
	nvs_close(nvshandle);
}

void read_flash()
{

		esp_err_t err = nvs_open("config", NVS_READONLY, &nvshandle);
			if(err!=ESP_OK)
				printf("Error opening NVS File\n");

			delay(300);
		 int largo=sizeof(aqui);
			err=nvs_get_blob(nvshandle,"config",(void*)&aqui,&largo);

		if (err !=ESP_OK)
			printf("Error read %d\n",err);
		nvs_close(nvshandle);
}

void erase_all_timers()
{
esp_err_t q;
//change to erase
	q=nvs_erase_all(knvshandle);

	for(int a=0;a<aqui.ucount;a++)
	{
		if(startTimer[a]){ //sanity check else crash
			  xTimerStop(startTimer[a],0);
			  xTimerDelete(startTimer[a],0);
					}
		if(endTimer[a]){//sanity check else crash
			   xTimerStop(endTimer[a],0);
			   xTimerDelete(endTimer[a],0);
					}

	}
		aqui.ucount=0;
		memset(&aqui.timerNames,0,sizeof(aqui.timerNames));
		memset(&startTimer,0,sizeof(startTimer));
		memset(&endTimer,0,sizeof(endTimer));

		write_to_flash();

		 q = nvs_commit(knvshandle);
		 if (q !=ESP_OK)
		 		printf("Commit ErrorKbd %d\n",q);
		 if(tankHandle)
			 vTaskDelete(tankHandle);
		 globalTimer=NULL;
}

float get_temp(u8 cualSensor)
		{
	if (cualSensor>numsensors-1)
		cualSensor=0;
	float tTemp=0.0;
	u8 vant=10;
	while(vant--)
	{
		tTemp=DS_get_temp(&sensors[cualSensor][0]);
		if (tTemp>1.0 && tTemp<200.0)
			return tTemp;
		delay(300);
	}
		return -1.0;
		}

void get_heater_name()
{
	char local[20];
	string appn;
	u8  mac[6];

	if (aqui.ssid[0]==0)
		esp_wifi_get_mac(WIFI_IF_AP, mac);
	else
		esp_wifi_get_mac(WIFI_IF_STA, mac);

	sprintf(local,"%02x%02x",mac[4],mac[5]);//last tow bytes of MAC to identify the connection
	string macID=string(local);
	for(int i = 0; i < macID.length(); i++)
		macID[i] = toupper(macID[i]);

	appn=string(aqui.heaterName);//heater name will be used as SSID if available else [APP]IoT-XXXX

	if (appn.length()<2)
	{
		AP_NameString = string(APP)+"-" + macID;
		appn=string(AP_NameString);
	}
	else
	{
		//	AP_NameString = appn +"-"+ macID;
		AP_NameString = appn ;
	}
	macID="";
	appn="";

}

// Convert a Mongoose string type to a string.
char *mgStrToStr(struct mg_str mgStr) {
	char *retStr = (char *) malloc(mgStr.len + 1);
	memcpy(retStr, mgStr.p, mgStr.len);
	retStr[mgStr.len] = 0;
	return retStr;
} // mgStrToStr

string getParameter(arg* argument, string cual)
{
	char paramr[50];

	if (argument->typeMsg ==1) //Json get parameter cual
	{
	//	cJSON *algo=(cJSON*)argument->pMessage;
	//	printf("type 1  busca %s algo %p\n",cual.c_str(),algo);
		cJSON *param= cJSON_GetObjectItem((cJSON*)argument->pMessage,cual.c_str());
		if(param)
			return string(param->valuestring);
		else
			return string("");
	}
	else //standard web server parameter
	{
	//	printf("Web look %s\n",cual.c_str());
		struct http_message * param=(struct http_message *)argument->pMessage;
		int a= mg_get_http_var(&param->query_string, cual.c_str(), paramr,sizeof(paramr));
		if(a>=0)
			paramr[a]=0;
		return string(paramr);
	}
	return "";
}

int findCommand(string cual)
{
	if(aqui.traceflag & (1<<CMDD))
		printf("[CMDD]Find %s of %d cmds\n",cual.c_str(),MAXCMDS);
	for (int a=0;a<MAXCMDS;a++)
		if(cual==string(cmds[a].comando))
			return a;
	return -1;
}


void webCmds(void * nc,struct http_message * params)
{
	arg *argument=(arg*)malloc(sizeof(arg));
	char *uri=mgStrToStr(params->uri);
	int cualf=findCommand(uri);
	if(cualf>=0)
	{
		if(aqui.traceflag & (1<<CMDD))
			printf("[CMDD]Webcmdrsn %d %s\n",cualf,uri);
		argument->pMessage=(void*)params;
		argument->typeMsg=0;
		argument->pComm=nc;
		(*cmds[cualf].code)(argument);
	//	xTaskCreate(cmds[cualf].code,"cmds",10000,(void*)argument, (configMAX_PRIORITIES - 1),NULL );
	}
	free(uri);
	free(argument);
}

void processCmds(void * nc,cJSON * comands)
{
//			if(aqui.traceflag & (1<<HEAPD))
//				printf(" Heap ProcessStart %d\n",xPortGetFreeHeapSize());
			cJSON *cmd= cJSON_GetObjectItem(comands,"cmd");
			if(cmd!=NULL)
			{
				int cualf=findCommand(string(cmd->valuestring));
				if(cualf>=0)
				{
					arg *argument=(arg*)malloc(sizeof(arg)); //this pointer will be freed by task itslef Do not free it here since it is still in process.
				//	printf(" Heap ProcessMalloc %d need %d\n",xPortGetFreeHeapSize(),sizeof(arg));

					argument->pMessage=(void*)comands;
				//	printf("CmdIteml %p=%s\n",argument->pMessage,cJSON_GetObjectItem(cmdIteml,"uid")->valuestring);
					argument->typeMsg=1;
					argument->pComm=nc;
					argument->theRoot=comands;
				//	xTaskCreate(cmds[cualf].code,"cmds",10000,(void*)argument, MGOS_TASK_PRIORITY,NULL );
					(*cmds[cualf].code)(argument);
					cJSON_Delete(comands);
					free(argument);
		//			printf(" Heap ProcessDone %d\n",xPortGetFreeHeapSize());

				}
				else
					if(aqui.traceflag & (1<<CMDD))
						printf("[CMDD]Cmd Not found\n");
			}
}

void sendResponse( void * comm,int msgTipo,string que,int len,int code,bool withHeaders, bool retain)
{
	int msg_id ;

	if(aqui.traceflag & (1<<PUBSUBD))
		printf("[PUBSUBD]Type %d Sending response [%s] len=%d\n",msgTipo,que.c_str(),que.length());

	if(msgTipo==1)
	{ // MQTT Response
		 esp_mqtt_client_handle_t mcomm=( esp_mqtt_client_handle_t)comm;

		if (!mqttf)
		{
			if(aqui.traceflag & (1<<MQTTD))
				printf("[MQTTD]No mqtt\n");
			return;
		}

	//	if(withHeaders)
			if(1)
		{
			for (int a=0;a<sonUid;a++)
			{
				spublishTopic="";
				if(montonUid[a]!="")
					spublishTopic=string(APP)+"/"+string(aqui.groupName)+"/"+string(aqui.heaterName)+"/"+montonUid[a]+"/MSG";
				else
					spublishTopic=string(APP)+"/"+string(aqui.groupName)+"/"+string(aqui.heaterName)+"/MSG";
				if(aqui.traceflag & (1<<PUBSUBD))
					printf("[PUBSUBD]Publish %s Msg %s for %d\n",spublishTopic.c_str(),que.c_str(),(u32)mcomm);
		//					printf(" Heap Sendb %d\n",xPortGetFreeHeapSize());
				 msg_id = esp_mqtt_client_publish(mcomm, (char*)spublishTopic.c_str(), (char*)que.c_str(),que.length(), 0, 0);
						//		printf(" Heap SendA %d\n",xPortGetFreeHeapSize());
				 if(msg_id<0)
					 printf("Error publish %d\n",msg_id);
				delay(200); //wait a while for next destination
			}
		}
		else
		{
				spublishTopic="";
				spublishTopic=string(APP)+"/"+string(aqui.groupName)+"/"+string(aqui.heaterName)+"/MSG";
//				if(aqui.traceflag & (1<<PUBSUBD))
//					printf("[PUBSUBD]DirectPublish %s Msg %s\n",spublishTopic.c_str(),que.c_str());
				msg_id = esp_mqtt_client_publish(mcomm, (char*)spublishTopic.c_str(), (char*)que.c_str(),que.length(), 0, 0);
				que="";

		}

	}
	else
	{ //Web Response
		struct mg_connection *nc=(struct mg_connection*)comm;
		if(aqui.traceflag & (1<<WEBD))
			printf("[WEBD]Web response nc %d\n",(u32)nc);

		if(len==0)
		{
			que=" ";
			len=1;
}
		mg_send_head(nc, 200, len, withHeaders?"Content-Type: text/html":"Content-Type: text/plain");
		mg_printf(nc, "%s", que.c_str());
		nc->flags |= MG_F_SEND_AND_CLOSE;
	}
}

void relay(u8 estado)
{
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Heater relay %d\n",estado);
	gpio_set_level((gpio_num_t)RELAY, estado);
//	printf(" Heap RelayBefore %d\n",xPortGetFreeHeapSize());

	if(xSemaphoreTake(I2CSem, portMAX_DELAY))
	{
		if(estado)
			drawString(RELAYX, RELAYY, "R", 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
		else
		{
			if(aqui.traceflag & (1<<GEND))
					printf("[GEND]Heater Off \n");
			drawString(RELAYX, RELAYY, "   ", 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
		}
		xSemaphoreGive(I2CSem);
	}
	//printf(" Heap Relayafter %d\n",xPortGetFreeHeapSize());

}
/*
static void IRAM_ATTR gpio_isr_handler(interrupt_type* arg)
{

	BaseType_t tasker;

//	if(!aqui.working)
//		return; //Nothing happening
	gpulseCount++;

//	if(arg->pin==DSPIN)
//	{
		//Logic for detecting Direction, Clockwise or CounterClockwise
//		motorp[pulsecnt]=millis();
//		motort.timestamp=motorp[pulsecnt];
//		if(pulsecnt==0)
//			basePulse=motorp[0];
//		motorp[pulsecnt]-=basePulse; //substract start
//		pulsecnt++;
//		if(pulsecnt>3)
//		{
//			memcpy(motorcop,motorp,sizeof(motorcop));
//			pulsecnt=0;
//			//sendSemaphore to DirectionTask
//			xSemaphoreGiveFromISR(dirSema, &tasker );
//					if (tasker)
//						portYIELD_FROM_ISR();
//		}
//		gMotorCount++;
//
//		return;
//	}
//	if(arg->mimutex && arg->pin==LASERSW)
//	{
//		args[2].timinter=0; //Wake LaserManager with Option 0 (standard laser break)
//		xSemaphoreGiveFromISR(arg->mimutex, &tasker );
//		if (tasker)
//			portYIELD_FROM_ISR();
//				}
}

*/
uint32_t readADC()
{
	u32 adc_reading=0;

    for (int i = 0; i < SAMPLES; i++)
    	adc_reading += adc1_get_raw((adc1_channel_t)adcchannel);

        adc_reading /= SAMPLES;
	return adc_reading;
}

void initSensors()
{
	gpio_config_t io_conf;
	uint64_t mask=1;  //If we are going to use Pins >=32 needs to shift left more than 32 bits which the compilers associates with a const 1<<x. Stupid

	 	io_conf.intr_type = GPIO_INTR_DISABLE;
	 	io_conf.pin_bit_mask = (mask<<DSPIN);
		io_conf.mode = GPIO_MODE_INPUT;
		io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
		io_conf.pull_up_en =GPIO_PULLUP_DISABLE;
		gpio_config(&io_conf);

		io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
		io_conf.pull_up_en =GPIO_PULLUP_DISABLE;
		io_conf.pin_bit_mask = (mask<<RELAY);
		io_conf.mode = GPIO_MODE_OUTPUT;
		gpio_config(&io_conf);

		io_conf.pin_bit_mask = (mask<<WIFILED);
		io_conf.mode = GPIO_MODE_OUTPUT;
		gpio_config(&io_conf);

		io_conf.pin_bit_mask = (mask<<MQTTLED);
		io_conf.mode = GPIO_MODE_OUTPUT;
		gpio_config(&io_conf);

		io_conf.pin_bit_mask = (mask<<WPIN);
		io_conf.mode = GPIO_MODE_OUTPUT;
		io_conf.pull_up_en =GPIO_PULLUP_ENABLE;
		gpio_config(&io_conf);

		gpio_set_level((gpio_num_t)WPIN, 1);

		gpio_set_level((gpio_num_t)RELAY, 0);
		gpio_set_level((gpio_num_t)WIFILED, 0);
		gpio_set_level((gpio_num_t)MQTTLED, 0);

//		// ADC setup will use ADC1 fixed Default VREF
	    adc1_config_width(ADC_WIDTH_BIT_12);
	    adcchannel=(adc1_channel_t)ADCCHAN;
	    adc1_config_channel_atten(adcchannel, ADC_ATTEN_DB_0); //1100 mv is the target, 10k and 5K volt divider is 1100mv

//	    //Characterize ADC
	    adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
	 //   esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}


double calcIrms(u16 Number_of_Samples,double ICAL,u32 SupplyVoltage )
{
//	SupplyVoltage=1100;
    u32 sampleI;
 //   u32 startt=millis();
    double filteredI,sqI,sumI=0.0;
    offsetI= (double)(ADC_COUNTS>>1); //half it THIS is calculated in Initsensors to get offset as read from ADC without load.
    //read them first for greater accuracy
  for (unsigned int n = 0; n < Number_of_Samples; n++)
      ampsMonton[n]=readADC();


  for(int n=0; n < Number_of_Samples; n++)
  {
    sampleI = ampsMonton[n];

    // Digital low pass filter extracts the 0.55 V dc offset,
    //  then subtract this - signal is now centered on 0 counts.
    offsetI = (offsetI + (sampleI-offsetI)/ADC_COUNTS);
    filteredI = sampleI - offsetI;

    // Root-mean-square method current
    // 1) square current values
    sqI = filteredI * filteredI;
    // 2) sum
    sumI += sqI;
  }

  double I_RATIO = aqui.calib *((SupplyVoltage/1000.0) / (ADC_COUNTS));
  double Irms = I_RATIO * sqrt((double)(sumI / Number_of_Samples));

  sumI = 0;
  //--------------------------------------------------------------------------------------

  return Irms;
}

char *mongoose_eventToString(int ev) {
	static char temp[100];
	switch (ev) {
	case MG_EV_CONNECT:
		return (char*)"MG_EV_CONNECT";
	case MG_EV_ACCEPT:
		return (char*)"MG_EV_ACCEPT";
	case MG_EV_CLOSE:
		return (char*)"MG_EV_CLOSE";
	case MG_EV_SEND:
		return (char*)"MG_EV_SEND";
	case MG_EV_RECV:
		return (char*)"MG_EV_RECV";
	case MG_EV_HTTP_REQUEST:
		return (char*)"MG_EV_HTTP_REQUEST";
	case MG_EV_HTTP_REPLY:
		return (char*)"MG_EV_HTTP_REPLY";
	case MG_EV_MQTT_CONNACK:
		return (char*)"MG_EV_MQTT_CONNACK";
	case MG_EV_MQTT_CONNACK_ACCEPTED:
		return (char*)"MG_EV_MQTT_CONNACK";
	case MG_EV_MQTT_CONNECT:
		return (char*)"MG_EV_MQTT_CONNECT";
	case MG_EV_MQTT_DISCONNECT:
		return (char*)"MG_EV_MQTT_DISCONNECT";
	case MG_EV_MQTT_PINGREQ:
		return (char*)"MG_EV_MQTT_PINGREQ";
	case MG_EV_MQTT_PINGRESP:
		return (char*)"MG_EV_MQTT_PINGRESP";
	case MG_EV_MQTT_PUBACK:
		return (char*)"MG_EV_MQTT_PUBACK";
	case MG_EV_MQTT_PUBCOMP:
		return (char*)"MG_EV_MQTT_PUBCOMP";
	case MG_EV_MQTT_PUBLISH:
		return (char*)"MG_EV_MQTT_PUBLISH";
	case MG_EV_MQTT_PUBREC:
		return (char*)"MG_EV_MQTT_PUBREC";
	case MG_EV_MQTT_PUBREL:
		return (char*)"MG_EV_MQTT_PUBREL";
	case MG_EV_MQTT_SUBACK:
		return (char*)"MG_EV_MQTT_SUBACK";
	case MG_EV_MQTT_SUBSCRIBE:
		return (char*)"MG_EV_MQTT_SUBSCRIBE";
	case MG_EV_MQTT_UNSUBACK:
		return (char*)"MG_EV_MQTT_UNSUBACK";
	case MG_EV_MQTT_UNSUBSCRIBE:
		return (char*)"MG_EV_MQTT_UNSUBSCRIBE";
	case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
		return (char*)"MG_EV_WEBSOCKET_HANDSHAKE_REQUEST";
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
		return (char*)"MG_EV_WEBSOCKET_HANDSHAKE_DONE";
	case MG_EV_WEBSOCKET_FRAME:
		return (char*)"MG_EV_WEBSOCKET_FRAME";
	}
	sprintf(temp, "Unknown event: %d", ev);
	return temp;
} //eventToString

void mongoose_event_handler(struct mg_connection *nc, int ev, void *evData) {
	//	char * evento=mongoose_eventToString(ev);
	//	printf("Event %s\n",evento);
	switch (ev) {
	case MG_EV_HTTP_REQUEST:
	{
		struct http_message *message = (struct http_message *) evData;
		if(aqui.traceflag&(1<<WEBD)){
			char *uri = mgStrToStr(message->uri);
			printf("cmd in http %s\n",uri);
		}

		//	webCmds(uri,mgStrToStr(message->query_string));
		webCmds((void*)nc,message);
		break;
		//	char *paramr=(char*)malloc(20);

	//	nc->flags |= MG_F_SEND_AND_CLOSE;
		break;
	}
	//	default:
	//		printf("Mong %d\n",ev);
	//		nc->flags |= MG_F_SEND_AND_CLOSE;
	//				break;
	}
} // End of mongoose_event_handler

void mongooseTask(void *data) {
	mongf=true;
	mg_mgr_init(&mgr, NULL);
	struct mg_connection *c = mg_bind(&mgr, ":80", mongoose_event_handler);
	if (c == NULL) {
		printf( "No connection from the mg_bind()\n");
			vTaskDelete(NULL);
		return;
	}
	mg_set_protocol_http_websocket(c);
	if(aqui.traceflag&(1<<BOOTD))
		printf("[BOOTD]Started mongoose\n");
	while (1)
		mg_mgr_poll(&mgr, 10);

} // mongooseTask

void mdnstask(void *args){
	char textl[60];
	time_t now;
	struct tm timeinfo;
	esp_err_t ret;
	time(&now);
	localtime_r(&now, &timeinfo);

		esp_err_t err = mdns_init();
		if (err) {
			printf( "Failed starting MDNS: %u\n", err);
		}
		else
		{
			if(aqui.traceflag&(1<<CMDD))
				printf("[CMDD]MDNS hostname %s\n",AP_NameString.c_str());
			mdns_hostname_set(AP_NameString.c_str()) ;
			mdns_instance_name_set(AP_NameString.c_str()) ;

			  mdns_txt_item_t serviceTxtData[4] = {
			        {(char*)"WiFi",(char*)"Yes"},
			        {(char*)"ApMode",(char*)"Yes"},
			        {(char*)"OTA",(char*)"Yes"},
			        {(char*)"Boot",(char*)""}
			    };

			sprintf(textl,"%d/%d/%d %d:%d:%d",1900+timeinfo.tm_year,timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);

			if(aqui.traceflag&(1<<CMDD))
				printf("[CMDD]MDNS time %s\n",textl);
			string s=string(textl);
			ret=mdns_service_add( AP_NameString.c_str(),"_heaterIoT", "_tcp", 80,serviceTxtData, 4 );
			if(ret && (aqui.traceflag&(1<<CMDD)))
							printf("Failed add service  %d\n",ret);
			ret=mdns_service_txt_item_set("_heaterIoT", "_tcp", "Boot", s.c_str());
			if(ret && (aqui.traceflag&(1<<CMDD)))
							printf("Failed add txt %d\n",ret);
		//	query_mdns_service(mdns, "_http", 0);

			s="";
		}

	vTaskDelete(NULL);
}

void initialize_sntp(void *args)
{
	 struct timeval tvStart;
//	struct tm mitime;
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, (char*)"pool.ntp.org");
	sntp_init();
	time_t now = 0;


	int retry = 0;
	const int retry_count = 10;
	setenv("TZ", "EST5", 1); //UTC is 5 hours ahead for Quito
	tzset();

	struct tm timeinfo;// = { 0 };
		timeinfo.tm_hour=timeinfo.tm_min=timeinfo.tm_sec=0;
		timeinfo.tm_mday=1;
		timeinfo.tm_mon=0;
		timeinfo.tm_year=100;
		// set to 1/1/2000 0:0:0
		magicNumber = mktime(&timeinfo); // magic number for timers .Sec  at above date to substract to all set timers to know secs to fire the
		// timer *1000 im ms

	while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
	//	ESP_LOGI(TAGG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		time(&now);
		localtime_r(&now, &timeinfo);
	}
	gettimeofday(&tvStart, NULL);
	sntpf=true;
	if(aqui.traceflag&(1<<BOOTD))
		printf("[BOOTD]Internet Time %04d/%02d/%02d %02d:%02d:%02d YDays %d DoW:%d\n",1900+timeinfo.tm_year,timeinfo.tm_mon,timeinfo.tm_mday,
			timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,timeinfo.tm_yday,timeinfo.tm_wday);
	gdayOfWeek=timeinfo.tm_wday;
//	printf("Magic Number: %ld Day:%d\n",magicNumber,gdayOfWeek); //Should be 946702800

	aqui.preLastTime=aqui.lastTime;
	time(&aqui.lastTime);
	write_to_flash();
	timef=1;
	postLog(0,aqui.bootcount,"Boot");

	if(!mdnsf)
		xTaskCreate(&mdnstask, "mdns", 4096, NULL, 5, NULL); //Ota Interface Controller
	//release this task
	loadTimers();
	vTaskDelete(NULL);
}


void ConfigSystem(void *pArg)
{
	uint32_t del=(uint32_t)pArg;
	while(1)
	{
		gpio_set_level((gpio_num_t)WIFILED, 1);
		delay(del);
		gpio_set_level((gpio_num_t)WIFILED, 0);
		delay(del);
	}
}


void newSSID(void *pArg)
{
	string temp;
	wifi_config_t sta_config;
	int len,cual=(int)pArg;

	len=0;
	esp_wifi_stop();

	temp=string(aqui.ssid[cual]);
	len=temp.length();

	if(xSemaphoreTake(I2CSem, portMAX_DELAY))
		{
			drawString(64,42,"               ",10,TEXT_ALIGN_CENTER,DISPLAYIT,REPLACE);
			drawString(64,42,string(aqui.ssid[curSSID]),10,TEXT_ALIGN_CENTER,DISPLAYIT,REPLACE);
			xSemaphoreGive(I2CSem);
		}

	if(aqui.traceflag & (1<<WIFID))
			printf("[WIFID]Try SSID =%s= %d %d\n",temp.c_str(),cual,len);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	temp=string(aqui.ssid[cual]);
	len=temp.length();
	memcpy((void*)sta_config.sta.ssid,temp.c_str(),len);
	sta_config.sta.ssid[len]=0;
	temp=string(aqui.pass[cual]);
	len=temp.length();
	memcpy((void*)sta_config.sta.password,temp.c_str(),len);
	sta_config.sta.bssid_set=0;
	sta_config.sta.password[len]=0;
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	esp_wifi_start(); //if error try again indefinitly

	vTaskDelete(NULL);

}
//test de simple

esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
	string local="Closed",temp;
//	int len;
//	char textl[50];

	if(aqui.traceflag & (1<<WIFID))
		printf("[WIFID]Wifi Handler %d\n",event->event_id);
    mdns_handle_system_event(ctx, event);

	//delay(100);
	switch(event->event_id)
	{
	case SYSTEM_EVENT_STA_GOT_IP:
	//	len=10;
		gpio_set_level((gpio_num_t)WIFILED, 1);
		connf=true;
		localIp=event->event_info.got_ip.ip_info.ip;
		get_heater_name();
		if(aqui.traceflag&(1<<BOOTD))
			printf( "[BOOTD]Got IP: %d.%d.%d.%d Mqttf %d\n", IP2STR(&event->event_info.got_ip.ip_info.ip),mqttf);

		if(!mqttf)
		{
			if(aqui.traceflag&(1<<CMDD))
				printf("[CMDD]Connect to mqtt\n");
			xTaskCreate(&mqttmanager,"mgr",10240,NULL,  5, NULL);		// User interface while in development. Erased in RELEASE

			clientCloud = esp_mqtt_client_init(&settings);
			 if(clientCloud)
			    esp_mqtt_client_start(clientCloud);
			 else
				 printf("Fail mqtt initCloud\n");
				 clientThing = esp_mqtt_client_init(&settingsThing);
				 if(clientThing)
				    esp_mqtt_client_start(clientThing);
				 else
					 printf("Fail mqtt init Thing\n");
		}

		if(!mongf)
		{
			if(I2CSem)
			{
				if(xSemaphoreTake(I2CSem, 1000))
				{
					setLogo("HeatIoT");
					xSemaphoreGive(I2CSem);
				}
			}
			xTaskCreate(&mongooseTask, "mongooseTask", 10240, NULL, 5, NULL); //  web commands Interface controller
			xTaskCreate(&initialize_sntp, "sntp", 2048, NULL, 3, NULL); //will get date
		}
		break;
	case SYSTEM_EVENT_AP_START:  // Handle the AP start event
		tcpip_adapter_ip_info_t ip_info;
		tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
		printf("System not Configured. Use local AP and IP:" IPSTR "\n", IP2STR(&ip_info.ip));

		if(!mongf)
		{
			xTaskCreate(&mongooseTask, "mongooseTask", 10240, NULL, 5, NULL);
			xTaskCreate(&initialize_sntp, "sntp", 2048, NULL, 3, NULL);
			xTaskCreate(&ConfigSystem, "cfg", 1024, (void*)100, 3, NULL);
		}
		break;

	case SYSTEM_EVENT_STA_START:
		if(aqui.traceflag & (1<<WIFID))
			printf("[WIFID]Connect\n");
		esp_wifi_connect();
		break;

	case SYSTEM_EVENT_STA_DISCONNECTED:
	case SYSTEM_EVENT_AP_STADISCONNECTED:
	case SYSTEM_EVENT_ETH_DISCONNECTED:
		connf=false;
		gpio_set_level((gpio_num_t)WIFILED, 0);

		if(aqui.traceflag & (1<<WIFID))
			printf("[WIFID]Reconnect %d\n",curSSID);
		curSSID++;
		if(curSSID>4)
			curSSID=0;

		temp=string(aqui.ssid[curSSID]);

		if(aqui.traceflag & (1<<WIFID))
			printf("[WIFID]Temp[%d]==%s=\n",curSSID,temp.c_str());
		if(temp!="")
		{
			xTaskCreate(&newSSID,"newssid",2048,(void*)curSSID, MGOS_TASK_PRIORITY, NULL);
		}
		else
		{
			curSSID=0;
			xTaskCreate(&newSSID,"newssid",2048,(void*)curSSID, MGOS_TASK_PRIORITY, NULL);
		}
		break;

	case SYSTEM_EVENT_STA_CONNECTED:
		if(aqui.traceflag & (1<<WIFID))
			printf("[WIFID]Connected SSID[%d]=%s\n",curSSID,aqui.ssid[curSSID]);
		aqui.lastSSID=curSSID;
		write_to_flash();
		gpio_set_level((gpio_num_t)WIFILED, 1);

		break;

	default:
		if(aqui.traceflag & (1<<WIFID))
			printf("[WIFID]default WiFi %d\n",event->event_id);
		break;
	}
	return ESP_OK;
} // wifi_event_handler

void initI2C()
{
	i2cp.sdaport=(gpio_num_t)SDAW;
	i2cp.sclport=(gpio_num_t)SCLW;
	i2cp.i2cport=I2C_NUM_0;
	miI2C.init(i2cp.i2cport,i2cp.sdaport,i2cp.sclport,400000,&I2CSem);//Will reserve a Semaphore for Control
}

void initWiFi(void *pArg)
{
	string temp;
	int len;
	wifi_init_config_t 				cfg=WIFI_INIT_CONFIG_DEFAULT();
	wifi_config_t 					sta_config,configap;
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	if (aqui.ssid[curSSID][0]!=0)
	{
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		temp=string(aqui.ssid[curSSID]);
		len=temp.length();
		memcpy(sta_config.sta.ssid,temp.c_str(),len+1);
		temp=string(aqui.pass[curSSID]);
		len=temp.length();
		memcpy(sta_config.sta.password,temp.c_str(),len+1);
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
	}

	else
	{
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
		memcpy(configap.ap.ssid,"HeaterIoT\0",10);
		memcpy(configap.ap.password,"csttpstt\0",9);
		configap.ap.ssid[11]=0;
		configap.ap.password[8]=0;
		configap.ap.ssid_len=0;
		configap.ap.authmode=WIFI_AUTH_WPA_PSK;
		configap.ap.ssid_hidden=false;
		configap.ap.max_connection=4;
		configap.ap.beacon_interval=100;
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &configap));
	}

	ESP_ERROR_CHECK(esp_wifi_start());

	vTaskDelete(NULL);
}

void initScreen()
{
	if(xSemaphoreTake(I2CSem, portMAX_DELAY))
	{
		display.init();
		display.flipScreenVertically();
		display.clear();
		drawString(64,18,"WiFi",24,TEXT_ALIGN_CENTER,NODISPLAY,NOREP);
		drawString(64,42,"               ",10,TEXT_ALIGN_CENTER,NODISPLAY,NOREP);
		drawString(64,42,string(aqui.ssid[curSSID]),10,TEXT_ALIGN_CENTER,DISPLAYIT,NOREP);
		xSemaphoreGive(I2CSem);
	}
	else
		printf("Failed to InitScreen\n");
}

void datacallback(esp_mqtt_client_handle_t self, esp_mqtt_event_handle_t event)
{
	mqttMsg mensa;
//	esp_err_t err;
//	printf(" Heap MsgIn %d need %d\n",xPortGetFreeHeapSize(),sizeof(mqttMsg));

//	mensa=(mqttMsg*)malloc(sizeof(mqttMsg));
//	printf(" Heap MsgMal %d\n",xPortGetFreeHeapSize());

	mensa.mensaje=(char*)event->data;
	mensa.mensaje[event->data_len]=0;
	mensa.nc=self;
	mensa.sizer=event->data_len;
	xQueueSend( mqttQ, ( void * ) &mensa,( TickType_t ) 0 );
//	printf(" Heap MsgQueue %d\n",xPortGetFreeHeapSize());

}

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
 //   int msg_id=0;
    esp_err_t err;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            if(client==clientCloud){
            	esp_mqtt_client_subscribe(client, cmdTopic.c_str(), 0);
        		gpio_set_level((gpio_num_t)MQTTLED, 1);
            }
            else
        		mqttThingf=true;
        	if(aqui.traceflag & (1<<MQTTD))
        		printf("[MQTTD]Connected %s(%d)\n",(char*)event->user_context,(u32)client);
            break;
        case MQTT_EVENT_DISCONNECTED:
        	if(aqui.traceflag & (1<<MQTTD))
        		printf( "[MQTTD]MQTT_EVENT_DISCONNECTED %s(%d)\n",(char*)event->user_context,(u32)client);
        	if(client==clientCloud){
        		gpio_set_level((gpio_num_t)MQTTLED, 0);
        		mqttf=false;
        	}
        	else
        		mqttThingf=false;

            break;

        case MQTT_EVENT_SUBSCRIBED:
        	if(client==clientThing)
        	{
            	if(aqui.traceflag & (1<<MQTTD))
            		printf( "[MQTTD]Subscribe ThingSpeak\n");
            	mqttThingf=true;
        	}
        	else{
            	if(aqui.traceflag & (1<<MQTTD))
            		printf("[MQTTD]Subscribed Cloud\n");
            	mqttf=true;
        		}
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
        	if(aqui.traceflag & (1<<MQTTD))
        		printf( "[MQTTD]MQTT_EVENT_UNSUBSCRIBED %s(%d)\n",(char*)event->user_context,(u32)client);
        	  if(client==clientCloud)
        		  esp_mqtt_client_subscribe(client, cmdTopic.c_str(), 0);
            break;
        case MQTT_EVENT_PUBLISHED:
        	if(aqui.traceflag & (1<<MQTTD))
        		printf( "[MQTTD]MQTT_EVENT_PUBLISHED %s(%d)\n",(char*)event->user_context,(u32)client);
            break;
        case MQTT_EVENT_DATA:
    //		printf("Event Heap Msg %d\n",xPortGetFreeHeapSize());
        	if(aqui.traceflag & (1<<MQTTD))
        	{
        		printf("[MQTTD]MSG for %s(%d)\n",(char*)event->user_context,(u32)client);
        		printf("[MQTTD]TOPIC=%.*s\r\n", event->topic_len, event->topic);
        		printf("[MQTTD]DATA=%.*s\r\n", event->data_len, event->data);
        	}
            if(client==clientCloud)
            	datacallback(client,event);
            break;
        case MQTT_EVENT_ERROR:
        	if(aqui.traceflag & (1<<MQTTD))
        		printf("[MQTTD]MQTT_EVENT_ERROR %s(%d)\n",(char*)event->user_context,(u32)client);
        	err=esp_mqtt_client_start(client);
        	if(err)
        	    printf("Error StartErr %s. None functional now!\n",(char*)event->user_context);
            break;
    }
    return ESP_OK;
}

void initVars()
{
	char textl[60];
	string idd;

	//We do it this way so we can have a single global.h file with EXTERN variables(when not main app)
	// and be able to compile routines in an independent file
	sonUid=0;


	uint16_t a=esp_random();
	sprintf(textl,"heater%04d",a);
	idd=string(textl);
	if(aqui.traceflag & (1<<BOOTD))
		printf("[BOOTD]Id %s\n",textl);

	settings.host=aqui.mqtt;
	settings.port = aqui.mqttport;
	settings.client_id=strdup(idd.c_str());
	printf("Client %s\n",settings.client_id);
	settings.username=aqui.mqttUser;
	settings.password=aqui.mqttPass;
	settings.event_handle = mqtt_event_handler;
	settings.user_context =aqui.mqtt; //name of server
	settings.transport=aqui.ssl?MQTT_TRANSPORT_OVER_SSL:MQTT_TRANSPORT_OVER_TCP;
	settings.buffer_size=2048;
	settings.disable_clean_session=true;


	settingsThing.host="mqtt.thingspeak.com";
	settingsThing.port=1883;
	settingsThing.event_handle = mqtt_event_handler;
	settingsThing.user_context =(void*)"mqtt.thingspeak.com"; //name of server
	strcpy(APP,"HeatIoT\0");

	strcpy(lookuptable[0].key,"BOOTD");
	strcpy(lookuptable[1].key,"WIFID");
	strcpy(lookuptable[2].key,"MQTTD");
	strcpy(lookuptable[3].key,"PUBSUBD");
	strcpy(lookuptable[4].key,"MONGOOSED");
	strcpy(lookuptable[5].key,"CMDD");
	strcpy(lookuptable[6].key,"WEBD");
	strcpy(lookuptable[7].key,"GEND");
	strcpy(lookuptable[8].key,"MQTTT");
	strcpy(lookuptable[9].key,"HEAPD");

	strcpy(lookuptable[10].key,"-BOOTD");
	strcpy(lookuptable[11].key,"-WIFID");
	strcpy(lookuptable[12].key,"-MQTTD");
	strcpy(lookuptable[13].key,"-PUBSUBD");
	strcpy(lookuptable[14].key,"-MONGOOSED");
	strcpy(lookuptable[15].key,"-CMDD");
	strcpy(lookuptable[16].key,"-WEBD");
	strcpy(lookuptable[17].key,"-GEND");
	strcpy(lookuptable[18].key,"-MQTTT");
	strcpy(lookuptable[19].key,"-HEAPD");

	for (int a=0;a<NKEYS;a++)
		if(a<(NKEYS/2))
			lookuptable[a].val=a;
		else
			lookuptable[a].val=a*-1;

	//Set up Mqtt Variables
	//spublishTopic=string(APP)+"/"+string(aqui.groupName)+"/"+string(aqui.heaterName)+"/MSG";
	cmdTopic=string(APP)+"/"+string(aqui.groupName)+"/"+string(aqui.heaterName)+"/CMD";
	topicString ="channels/514840/publish/ANF6WMYG2T7VWIMM";

	strcpy(meses[0],"Ene");
	strcpy(meses[1],"Feb");
	strcpy(meses[2],"Mar");
	strcpy(meses[3],"Abr");
	strcpy(meses[4],"May");
	strcpy(meses[5],"Jun");
	strcpy(meses[6],"Jul");
	strcpy(meses[7],"Ago");
	strcpy(meses[8],"Sep");
	strcpy(meses[9],"Oct");
	strcpy(meses[10],"Nov");
	strcpy(meses[11],"Dic");

	daysInMonth [0] =31;
	daysInMonth [1] =28;
	daysInMonth [2] =31;
	daysInMonth [3] =30;
	daysInMonth [4] =31;
	daysInMonth [5] =30;
	daysInMonth [6] =31;
	daysInMonth [7] =31;
	daysInMonth [8] =30;
	daysInMonth [9] =31;
	daysInMonth [10] =30;
	daysInMonth [11] =31;

	// set pairs of "command name" with Function to be called
	// OJO commandos son con el backslash incluido ej: /mt_HttpStatus y no mt_HttpStatus a secas!!!!

	strcpy((char*)&cmds[0].comando,"/ht_firmware");			cmds[0].code=set_FirmUpdateCmd;			//done...needs testing in a good esp32
	strcpy((char*)&cmds[1].comando,"/ht_erase");			cmds[1].code=set_eraseConfig;			//done
	strcpy((char*)cmds[2].comando,"/ht_status");			cmds[2].code=set_statusSend;			//done
	strcpy((char*)cmds[3].comando,"/ht_reset");				cmds[3].code=set_reset;					//done
	strcpy((char*)cmds[4].comando,"/ht_resetstats");		cmds[4].code=set_resetstats;			//done
	strcpy((char*)cmds[5].comando,"/ht_OnOff");				cmds[5].code=set_onoff;					//done
	strcpy((char*)cmds[6].comando,"/ht_generalap");			cmds[6].code=set_generalap;				//done
	strcpy((char*)cmds[7].comando,"/ht_scan");				cmds[7].code=set_scanCmd;				//done
	strcpy((char*)cmds[8].comando,"/ht_Zerousers");			cmds[8].code=set_zerousers;				//done
	strcpy((char*)cmds[9].comando,"/ht_timerAdd");			cmds[9].code=set_timeradd;				//done
	strcpy((char*)cmds[10].comando,"/ht_timerDel");			cmds[10].code=set_timerdel;				//done
	strcpy((char*)cmds[11].comando,"/ht_sync");				cmds[11].code=set_sync;					//done
	strcpy((char*)cmds[12].comando,"/ht_display");			cmds[12].code=set_display;				//done
	strcpy((char*)cmds[13].comando,"/ht_timerOnOff");		cmds[13].code=set_timeronoff;			//done
	strcpy((char*)cmds[14].comando,"/ht_mqtt");			 	cmds[14].code=set_mqtt;					//done
	strcpy((char*)cmds[15].comando,"/ht_gettimers");		cmds[15].code=set_gettimers;			//done
	strcpy((char*)cmds[16].comando,"/ht_manual");			cmds[16].code=set_manual;				//done
	strcpy((char*)cmds[17].comando,"/ht_ampscalib");		cmds[17].code=set_calibration;			//done
	strcpy((char*)cmds[18].comando,"/ht_readlog");			cmds[18].code=set_readlog;				//done


	barX[0]=0;
	barX[1]=6;
	barX[2]=12;

	barH[0]=5;
	barH[1]=10;
	barH[2]=15;

	strcpy((char*)WIFIME,"HeaterIoT0");

	displayf=true;
	mongf=false;
	//compile_date[] = __DATE__ " " __TIME__;

//	memset(&opts, 0, sizeof(opts));
//	opts.user_name =s_user_name ;
//	opts.password = s_password;
//	opts.keep_alive=1;

	logText[0]="System booted";
	logText[1]="Log CLeared";
	logText[2]="Firmware Updated";
	logText[3]="General Error";
	logText[4]="Start Timer";
	logText[5]="Log";
	logText[6]="Heater Reseted";
	logText[7]="Ap Set";
	logText[8]="Internal";
	logText[9]="End Timer";
	logText[10]="Timer Add";
	logText[11]="Timer Delete";
	logText[12]="Timer Sync";
	logText[13]="Heater Off";
	logText[14]="Heater On";
	logText[15]="Reload Reboot";
	logText[16]="Manual On";
	logText[17]="Manual Off";
	logText[18]="Heap Guard";

	gsleepTime=aqui.disptime*1000*60;  //value in minutes make it milisecs
	dispTimer=xTimerCreate("dispTimer",gsleepTime/portTICK_PERIOD_MS,pdFALSE,( void * ) 0,&timerCallback);

	if(dispTimer==NULL)
		printf("Failed to create Display timer\n");
}


int init_log()
{
	const char *base_path = "/spiflash";
	static wl_handle_t	s_wl_handle=WL_INVALID_HANDLE;
	esp_vfs_fat_mount_config_t mount_config={0,0,0};

	mount_config.max_files=1;
	mount_config.format_if_mount_failed=true;

	loggf=false;

	logQueue=xQueueCreate(10,sizeof(logq));
	// Create Queue
	if(logQueue==NULL)
		return -1;

	logSem= xSemaphoreCreateBinary();
	if(logSem)
		xSemaphoreGive(logSem);  //SUPER important else its born locked
	else
		printf("Cant allocate Log Sem\n");

	esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle);
	if (err != ESP_OK) {
		printf( "Failed to mount FATFS %d \n", err);
		return -1;
	}
	bitacora = fopen("/spiflash/log.txt", "r+");
	if (bitacora == NULL) {
		bitacora = fopen("/spiflash/log.txt", "a");
		if(bitacora==NULL)
		{
			if(aqui.traceflag&(1<<BOOTD))
			printf("[BOOTD]Could not open file\n");
			return -1;
		}
		else
			if(aqui.traceflag&(1<<BOOTD))
				printf("[BOOTD]Opened file append\n");
	}
	loggf=true;

	return ESP_OK;
}


void init_temp()
{
	//Temp sensors
	numsensors=DS_init(DSPIN,bit9,&sensors[0][0]);
	if(numsensors==0)
		numsensors=DS_init(DSPIN,bit9,&sensors[0][0]); //try again
	if(aqui.traceflag & (1<<BOOTD))
	{
		for (int a=0;a<numsensors;a++)
		{
			printf("[BOOTD]Sensor %d Id=",a);
			for (int b=0;b<8;b++)
				printf("%02x",sensors[a][b]);
			delay(750);
			if(numsensors>0)
			printf(" Temp:%.02fC\n",get_temp(a));
		}
	}
}


void tankManager(void *pArg)
{
	float tankTemp=0.0,theOtherTemp=0.0,ttemp;
	tickets *thisTimer=(tickets*)pArg;
	char textl[20];
	u32 segundos,lastMilisAmps,nowmilis,sonmilis;
	u16 horas,minutos,secsf,countTimesChanged;
	bool checkHys=false;
//	float flowrLMin,flowrMlSecs;
	string tempString;

	if(thisTimer==NULL)
	{
		printf("Tankmg null timer\n");
		return;
	}
	relay(ON); //Turn the heater ON
	countTimesChanged=0;

	lastMilisAmps=millis();
	int vant=0;

while(1)
{
	//oldtime=millis();

	delay(5000);//Every 5000 ms
	vant++;
	if(!aqui.oneTankf)
		ttemp=get_temp(aqui.lastTankId); //always needs to read temp to control it
	else
		ttemp=get_temp(0); //always needs to read temp to control it

	if(ttemp>0.0 && ttemp<85.0)
		tankTemp=ttemp;

	if(displayf)
		{
			if(xSemaphoreTake(I2CSem, portMAX_DELAY))
				{

					eraseMainScreen();
					drawString(0, 20, thisTimer->userName,24, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
					sprintf(textl,"%.01fC\n",tankTemp);
					drawString(98, 35, string(textl), 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
					if(startTimer[thisTimer->internalNumber])
						{
						//	printf("Start Timer active: %s End Timer Active:%s\n",xTimerIsTimerActive( startTimer[a] )?"Yes":"No",xTimerIsTimerActive( endTimer[a] )?"Yes":"No");
						//	 xRemainingTime = xTimerGetExpiryTime( startTimer[a] ) - xTaskGetTickCount();
						//	 printf("Time to remaining to Start:%d secs (%d mins) ",xRemainingTime/1000,xRemainingTime/60000);
							 segundos = xTimerGetExpiryTime( endTimer[thisTimer->internalNumber] ) - xTaskGetTickCount();
							 segundos /=1000; // in seconds now
							 horas =segundos/3600;
							 minutos= (segundos-(horas*3600))/60;
							 secsf=segundos-horas*3600-minutos*60;
							 sprintf(textl,"%02d:%02d:%02d",horas,minutos,secsf);
							 drawString(85,21, string(textl), 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
						}
					xSemaphoreGive(I2CSem);
				}
		}

	// control temp section

	if((int)tankTemp>=thisTimer->requiredTemp && !checkHys)
	{
		relay(OFF);//turn relay off
		checkHys=true;
		if(aqui.traceflag & (1<<GEND))
			printf("[GEND]Temp reached %.2f vs %d\n",tankTemp,thisTimer->requiredTemp);
	}
		if((int)tankTemp<(thisTimer->requiredTemp*95/100) && checkHys)
			{
				relay(ON);//turn relay off
				checkHys=false;
				if(aqui.traceflag & (1<<GEND))
					printf("[GEND]Restart Heater %.2f\n",tankTemp);
			}

		//check if AutoTemp is on.
		if(aqui.autoTank && !aqui.oneTankf)
		{
			theOtherTemp=get_temp(!aqui.lastTankId);
			//a "reasonable" distance must be between both to consider changing current selection at least 10 degrees.
			if((theOtherTemp-tankTemp>MINDIFTEMP))
			{
				//CHANGE IT. THis temp is higher by MINDIFTEMP
					countTimesChanged++;
					if(countTimesChanged<10) //guard against constantly chaing like an hw error.
					{
						aqui.lastTankId =!aqui.lastTankId;
						write_to_flash();
					}
			}

		}

// Amps calculations
		double amps;

		amps=calcIrms(100,0.0,1110);

		nowmilis=millis();
		sonmilis=nowmilis-lastMilisAmps; // miliseconds since last read
		lastMilisAmps=nowmilis;

		//Update stats
		// Amps section
		thisTimer->acumTime+=sonmilis;
		thisTimer->ampCount++;
		thisTimer->nowamp=amps;
		if (amps>thisTimer->maxAmp)
			thisTimer->maxAmp=amps;
		if (amps<thisTimer->minAmp)
			thisTimer->minAmp=amps;

		// kWh section
		// kwh=PF (1 here)*volts/1000*milisecs/3600000;

		thisTimer->nowkwh=amps*aqui.volts/1000.0*((double)sonmilis/3600000.0);
		thisTimer->kwh+=thisTimer->nowkwh;

		if(aqui.traceflag & (1<<GEND))
			printf("[GEND]AMP %.02f kWh %.05f ms %d\n",amps,thisTimer->nowkwh,sonmilis);

		//Water flow meter
		//
//			sonm=millis()-oldtime;
//
//			if(gpulseCount>0)
//			{
//				flowrLMin=(float)(1000.0/sonm*gpulseCount/4500.0); //liters per minute
//				flowrMlSecs=flowrLMin/60.0*1000.0;
//				if(aqui.traceflag & (1<<GEND))
//					printf("[GEND]Ms %d Pulse Count %4d FlowLMin %.04f  FlowMlSec %.04f\n",sonm,gpulseCount,flowrLMin,flowrMlSecs);
//				gpulseCount=0;
//				if(globalTimer)
//				{
//					globalTimer->litros=flowrLMin*(sonm/60000); // liters in alloted time
//					globalTimer->litermin=flowrLMin; //liters minute
//				}
//			}

}

}

void monitorTank(void *pArg)
{
	float tankT, ambT,oldT=0.0,oldA=0.0;
	string tempString;
	char textl[100];
	u32 looptime=20000;

	while(1)
	{
		if(aqui.monitorf)
		{
			looptime=aqui.monitorTime*3600*1000; // from Minutes to miliseconds. Make in updatable on the fly
			if (looptime<20000)
				looptime=20000; //just in case
			if(aqui.thingsChannel!=NULL && (aqui.monitorf) && mqttThingf)
			{
				if(!aqui.oneTankf)
				{
					tankT=get_temp(aqui.lastTankId); //always needs to read temp to control it
					if(tankT>0.0)
						oldT=tankT;
					ambT=get_temp(!aqui.lastTankId);
					if(ambT>0.0)
						oldA=ambT;
					sprintf(textl,"field1=%.02f&field2=%.02f",oldT,oldA);
				}
				else
				{
					tankT=get_temp(0); //always needs to read temp to control it
					sprintf(textl,"field1=%.02f",tankT);
				}
				tempString=string(textl);
				if(mqttThingf)
					esp_mqtt_client_publish(clientThing, topicString.c_str(), tempString.c_str(), tempString.length(), 0, 0);
				tempString="";
			}
		}
		delay(looptime);//algo
	}
}


void startHeaterCallback( TimerHandle_t xTimer )
{
	tickets *thisTimer;
	thisTimer= ( tickets* ) pvTimerGetTimerID( xTimer ); //update stats and whatever to this pointer. Wait for it to end to save it in nvs

	if(!aqui.working)
	{
		if(aqui.traceflag & (1<<GEND))
			printf("Timer %s not started. Heater Off\n",thisTimer->userName);
		return;
	}

	if(thisTimer==NULL)
	{
		printf("Start Null timer\n");
		return;
	}

	globalTimer=thisTimer; //for status send that cannot access a private pointer
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Start timer called %s\n",thisTimer->userName);
	thisTimer->starts++;
	time((time_t*)&thisTimer->TimeStart);

	if(!aqui.oneTankf)
		thisTimer->tempStart=(int)get_temp(aqui.lastTankId);
	else
		thisTimer->tempStart=(int)get_temp(0);

	postLog(4,thisTimer->tempStart,thisTimer->userName);

	// rewrite timer
	esp_err_t q ;
    q=nvs_set_blob(knvshandle,thisTimer->userName,(void*)thisTimer,sizeof(tickets)); //write a timer ticket name as Passed
    if (q !=ESP_OK)
         printf("Error Start rewriting timer %s\n",thisTimer->userName);
	xTaskCreate(&tankManager,"monitor",10240,thisTimer, MGOS_TASK_PRIORITY, &tankHandle);						// Log Manager

	if(!displayf){
    	display.displayOn();
    	displayf=true;
    	if(aqui.disptime>0)
    		xTimerStart(dispTimer,0);
    }

	}

void endHeaterCallback( TimerHandle_t xTimer )
{
	tickets *thisTimer;
	char textl[20];
	float theTemp;
	time_t now;

	time(&now);

	if(!displayf){
		display.displayOn();
		displayf=true;
    	if(aqui.disptime>0)
    		xTimerStart(dispTimer,0);	}

    relay(OFF);

	thisTimer= ( tickets* ) pvTimerGetTimerID( xTimer );
	if(thisTimer==NULL){
		printf("End null timer\n");
		return;
	}

	if(!aqui.working)
		{
			if(aqui.traceflag & (1<<GEND))
				printf("Timer %s never started. Heater Off\n",thisTimer->userName);
			return;
		}
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]End timer called %s elapsed %d\n",thisTimer->userName,((u32)now-globalTimer->TimeStart));

	if(!aqui.oneTankf)
		thisTimer->tempStop=(int)get_temp(aqui.lastTankId);
	else
		thisTimer->tempStop=(int)get_temp(0);

	time((time_t*)&thisTimer->TimeStop);
	postLog(9,thisTimer->tempStop,thisTimer->userName);
	esp_err_t q ;
	    q=nvs_set_blob(knvshandle,thisTimer->userName,(void*)thisTimer,sizeof(tickets)); //save timer data
	    if (q !=ESP_OK)
	         printf("Error End rewriting timer %s\n",thisTimer->userName);
		 q = nvs_commit(knvshandle);
		 if (q !=ESP_OK)
		 		printf("Commit Error endh %d\n",q);

		 if(tankHandle!=NULL)
			 vTaskDelete(tankHandle); //stop monitoring
	    tankHandle=NULL;

		if(!aqui.oneTankf)
		    theTemp=get_temp(aqui.lastTankId);
		else
			theTemp=get_temp(0);

	    sprintf(textl,"%.2f C",theTemp);

			if(xSemaphoreTake(I2CSem, portMAX_DELAY))
		{
			eraseMainScreen();
			drawString(0, 20, textl,24, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
			drawString(95, 30, thisTimer->userName,10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);

			xSemaphoreGive(I2CSem);
		}
		if(startTimer[thisTimer->internalNumber])
		{
			xTimerDelete(startTimer[thisTimer->internalNumber],0);
			xTimerDelete(endTimer[thisTimer->internalNumber],0);
			startTimer[thisTimer->internalNumber]=NULL;
			endTimer[thisTimer->internalNumber]=NULL;
		}
		globalTimer=NULL;
}
/*
uint8_t FindNsID ( const esp_partition_t* nvs, const char* ns )
{
  esp_err_t                 result = ESP_OK ;                 // Result of reading partition
  uint32_t                  offset = 0 ;                      // Offset in nvs partition
  uint8_t                   i ;                               // Index in Entry 0..125
  uint8_t                   bm ;                              // Bitmap for an entry
  uint8_t                   res = 0xFF,span ;                      // Function result

  while ( offset < nvs->size )
  {
	  printf("Offset %d\n",offset);
    result = esp_partition_read ( nvs, offset,                // Read 1 page in nvs partition
                                  &buf,
                                  sizeof(nvs_page) ) ;
    if ( result != ESP_OK )
    {
      printf ( "Error reading NVS!\n" ) ;
      break ;
    }
    i = 0 ;
    printf("Search %s offset %d page %d\n",ns,offset,sizeof(nvs_page));
    while ( i < 126 )
    {
      bm = ( buf.Bitmap[i/4] >> ( ( i % 4 ) * 2 ) ) & 0x03 ;  // Get bitmap for this entry
  	printf("BM[%d]=%x\n",i,bm);

      if ( ( bm == 2 ) &&
           ( buf.Entry[i].Ns == 0 ) &&
           ( strcmp ( ns, buf.Entry[i].Key ) == 0 ) )
      {
        res = buf.Entry[i].Data & 0xFF ;                      // Return the ID
        offset = nvs->size ;                                  // Stop outer loop as well
        break ;
      }
      else
      {
        if ( bm == 2 )
        {
        span=buf.Entry[i].Span ;
          i +=span;
          //buf.Entry[i].Span ;                             // Next entry
          printf("i %d span %d bm %d\n",i,span,bm);
        }
        else
        {
          i++ ;
    //      printf("i  %d\n",i);

        }
      }
    }
    offset += sizeof(nvs_page) ;                              // Prepare to read next page in nvs
  }
  return res ;
}

void findKeys()
{
  esp_partition_iterator_t  pi ;                              // Iterator for find
  esp_partition_t*    nvss ;                             // Pointer to partition struct
  esp_err_t                 result = ESP_OK ;
  char                      partname[10];
  uint8_t                   pagenr = 0 ;                      // Page number in NVS
  uint8_t                   i ;                               // Index in Entry 0..125
  uint8_t                   bm ;                              // Bitmap for an entry
  uint32_t                  offset = 0 ;                      // Offset in nvs partition
  uint8_t                   namespace_ID ;                    // Namespace ID found

 sprintf(partname,"%s","nvs");
  pi = esp_partition_find ( ESP_PARTITION_TYPE_DATA,          // Get partition iterator for
                            ESP_PARTITION_SUBTYPE_ANY,        // this partition
                            partname ) ;
  if ( pi )
  {
    nvss = esp_partition_get ( pi ) ;                          // Get partition struct
    esp_partition_iterator_release ( pi ) ;                   // Release the iterator
    printf("Part %s found %d bytes\n",partname,nvss->size);
  }
  else
  {
    printf ( "Partition %s not found!", partname ) ;
    return ;
  }
  namespace_ID = FindNsID ( nvss, "config" ) ;             // Find ID of our namespace in NVS
  printf ( "Namespace ID of config is %d\n",namespace_ID ) ;
  while ( offset < nvss->size )
  {
    result = esp_partition_read ( nvss, offset,                // Read 1 page in nvs partition
                                  &buf,
                                  sizeof(nvs_page) ) ;
    if ( result != ESP_OK )
    {
      printf ( "Error reading NVS!\n" ) ;
      return ;
    }
    printf ( "\n" ) ;
    printf ( "Dump page %d\n", pagenr ) ;
    printf ( "State is %08X\n", buf.State ) ;
    printf ( "Seqnr is %08X\n", buf.Seqnr ) ;
    printf ( "CRC   is %08X\n", buf.CRC ) ;
    i = 0 ;
    while ( i < 126 )
    {
      bm = ( buf.Bitmap[i/4] >> ( ( i % 4 ) * 2 ) ) & 0x03 ;  // Get bitmap for this entry
      if ( bm == 2 )
      {
        if ( ( namespace_ID == 0xFF ) ||                      // Show all if ID = 0xFF
             ( buf.Entry[i].Ns == namespace_ID ) )            // otherwise just my namespace
        {
          printf ( "Key %03d: %s\n", i,                       // Print entrynr
                     buf.Entry[i].Key ) ;                     // Print the key
        }
        i += buf.Entry[i].Span ;                              // Next entry
      }
      else
      {
        i++ ;
      }
    }
    offset += sizeof(nvs_page) ;                              // Prepare to read next page in nvs
    pagenr++ ;
  }
}
*/
void launchTimer(int a )
{
	string s;
	struct tm timeinfo = {  };
	time_t now;
	int startReal,endReal;

	int dia=1<<gdayOfWeek;
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]In LauchTimer for %d\n",a);
	time(&now); //get current time
		localtime_r(&now, &timeinfo); // convert to struct to change date to 2000/1/1

		timeinfo.tm_mday=1;
		timeinfo.tm_mon=0;
		timeinfo.tm_year=100;
				// set to 1/1/2000 x:y:z
		now = mktime(&timeinfo); // now with same date so only time counts
		s=string(aqui.timerNames[a]);
				esp_err_t q ;
					size_t largo=sizeof(tickets);
						q=nvs_get_blob(knvshandle,s.c_str(),(void*)&mitimer[a],&largo);

					if (q !=ESP_OK)
						printf("Error get timer config %s\n",s.c_str()); //could not find that name, try more if available. Internal error in theory
					else
					{
//						if(aqui.traceflag & (1<<GEND))
//							printf("[GEND]Timer %s day %d dia %d c1 %d c2 %d c3 %d add %ld now %ld\n",mitimer[a].userName,mitimer[a].days,dia,(mitimer[a].days & dia),mitimer[a].onOff,(mitimer[a].fromDate+mitimer[a].duration>now),mitimer[a].fromDate+mitimer[a].duration,now);

							if((mitimer[a].days & dia) && mitimer[a].onOff && (mitimer[a].fromDate+mitimer[a].duration>now))
						{
								mitimer[a].internalNumber=a; //for queue identification


							if(mitimer[a].fromDate<now)
							{
								startReal=1;
								endReal=mitimer[a].fromDate-now+mitimer[a].duration;

							}
							else
							{
								startReal=mitimer[a].fromDate-now;
								endReal=startReal+mitimer[a].duration;
							}

							if(aqui.traceflag & (1<<GEND))
								printf("[GEND]Start %s in %d end in %d f %ld d %ld now %d\n",mitimer[a].userName,startReal,endReal,mitimer[a].fromDate,mitimer[a].duration,(int)now);

							startTimer[a]=xTimerCreate(s.c_str(),startReal*1000 /portTICK_PERIOD_MS,pdFALSE,( void * ) &mitimer[a],&startHeaterCallback);
							if(startTimer[a]==NULL)
								printf("Failed to create Start timer %s\n",s.c_str());
							else
							{
								endTimer[a]=xTimerCreate(s.c_str(),endReal*1000 /portTICK_PERIOD_MS,pdFALSE,( void * ) &mitimer[a],&endHeaterCallback);
								if(endTimer[a]==NULL)
									printf("Failed to create End timer %s\n",s.c_str());
								else
									{
									//start both timers
										if( xTimerStart( startTimer[a],0) != pdPASS )
											printf("Failed to start starttimer %s\n",s.c_str());
										else
											if( xTimerStart( endTimer[a],0) != pdPASS )
												{
													printf("Failed to start endtimer %s\n",s.c_str());
													xTimerStop(startTimer[a],0);
												}
									}
							}
						}
					}

}

void loadTimers()
{
	// Load all Active On timers with their Off timers. Two different callbacks
	int fueron=0;
	string s;
	time_t t = 0;
	struct tm timeinfo ;

	time(&t);
	localtime_r(&t, &timeinfo);
	//Save day for Midnight restart
	todate=timeinfo.tm_yday; //day of the year when timers where launched

	for (int a=0; a<10;a++) //iterate over all possible timernames
	{
		s=string(aqui.timerNames[a]);
		if(s!="") //not empty load its data def
			{
				fueron++;
				launchTimer(a);
		}
		else
			break;
	}
	if(aqui.traceflag & (1<<GEND))
		printf("[GEND]Launched timers %d\n",fueron);
}
void wakeUp(void *pArg)
{
	while(1)
	{
		delay(1000);
		if(!displayf)
		{
			if(!gpio_get_level((gpio_num_t)0) )
			{
				if( xTimerIsTimerActive( dispTimer ))
					xTimerStop(dispTimer,0);
					display.displayOn();
					xTimerStart(dispTimer,0);
					displayf=true;
			}
		}
		else
		{
			if(!gpio_get_level((gpio_num_t)0) )
			{
				gsleepTime+=10000;
				if(gsleepTime>200000)
					gsleepTime=30000;
				xTimerDelete(dispTimer,0);
				dispTimer=xTimerCreate("dispTimer",gsleepTime/portTICK_PERIOD_MS,pdFALSE,( void * ) 0,&timerCallback);
				if(dispTimer)
					xTimerStart(dispTimer,0);
			}
		}
	}
}

void heaterOnOff(void *pArg)
{
	while(1)
	{
		if(aqui.working)
		{
			gpio_set_level((gpio_num_t)WPIN, 1);
			delay(1000);
		}
		else
		{
			gpio_set_level((gpio_num_t)WPIN, 0);
			delay(300);
			gpio_set_level((gpio_num_t)WPIN, 1);
			delay(300);
		}
	}
}

void heapWD(void *pArg)
{
	while(1)
	{
		delay(60000); //every minute
		if(xPortGetFreeHeapSize()<MINHEAP)
		{
			postLog(18,xPortGetFreeHeapSize(),0);
			delay(1000);
			esp_restart();
		}
	}
}

void app_main(void)
{
	esp_log_level_set("*", ESP_LOG_ERROR); //shut up
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

	//findKeys();k

	gpio_set_direction((gpio_num_t)0, GPIO_MODE_INPUT);
	delay(3000);

	// load configuration
	err = nvs_open("config", NVS_READWRITE, &nvshandle);
	if(err!=ESP_OK){
		printf("Error opening NVS File\n");
	}
	else
		nvs_close(nvshandle);


	// load timers
	err = nvs_open("timerk", NVS_READWRITE, &knvshandle);
	if(err!=ESP_OK)
		printf("Error opening NVS Timers File\n");
	//else
	//	nvs_close(knvshandle);



	rebootl= rtc_get_reset_reason(1); //Reset cause for CPU 1
	read_flash();

	if (aqui.centinel!=CENTINEL || !gpio_get_level((gpio_num_t)0))
		//	if (aqui.centinel!=CENTINEL )
	{
		printf("Read centinel %x",aqui.centinel);
		erase_config();
	}
    printf("Esp32-Heater\n");

	curSSID=aqui.lastSSID;
	initVars(); 			// used like this instead of var init to be able to have independent file per routine(s)
	initI2C();  			// for Screen and RTC clock
	initScreen();			// Screen
    init_temp();			// Temperature sensors should be two one for Ambient and the other for waterK
	initSensors();
	init_log();				// Log file management


	//Save new boot count and reset code
	aqui.bootcount++;
	aqui.lastResetCode=rebootl;
	write_to_flash();

	xTaskCreate(&initWiFi,"log",10240,NULL, MGOS_TASK_PRIORITY, NULL);						// Log Manager
//	initWiFi(NULL);

	// Start Main Tasks
	xTaskCreate(&displayManager,"dispMgr",10240,NULL, MGOS_TASK_PRIORITY, NULL);				//Manages all display to LCD
	xTaskCreate(&kbd,"kbd",8192,NULL, MGOS_TASK_PRIORITY, NULL);								// User interface while in development. Erased in RELEASE//
	xTaskCreate(&logManager,"log",4096,NULL, MGOS_TASK_PRIORITY, NULL);						// Log Manager
	xTaskCreate(&wakeUp,"wake",1024,NULL, MGOS_TASK_PRIORITY, NULL);						// Log Manager
	xTaskCreate(&monitorTank,"monitorT",8192,NULL, MGOS_TASK_PRIORITY, NULL);						// Log Manager

	xTaskCreate(&heaterOnOff,"active",4096,NULL, MGOS_TASK_PRIORITY, NULL);						// Log Manager
	xTaskCreate(&heapWD,"heapWD",1024,NULL, MGOS_TASK_PRIORITY, NULL);						// Log Manager



	if(aqui.disptime>0)
		xTimerStart(dispTimer,0);}
