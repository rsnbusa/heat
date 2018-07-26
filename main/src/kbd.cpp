/*
 * kbd.cpp
 *
 *  Created on: Apr 18, 2017
 *      Author: RSN
 */
#include "kbd.h"

extern void show_config(u8 meter, bool full);
extern void load_from_fram(u8 meter);
extern void write_to_flash();
void write_to_fram(u8 meter,bool adding);
extern string makeDateString(time_t t);
//extern mqtt_settings settings;
extern uart_port_t uart_num;
//typedef struct { char key[10]; int val; } t_symstruct;
EXTERN t_symstruct 	lookuptable[NKEYS];
//extern u8 laserLoop(u16 watil,u16 waitoff);
extern float get_temp(u8 cual);
extern void erase_all_timers();
extern void testAmps();
extern double calcIrms(u16 Number_of_Samples,double ICAL,u32 SupplyVoltage );
extern void relay(u8 estado);
extern string makeDateString(time_t t);

int keyfromstring(char *key)
{

    int i;
    for (i=0; i < NKEYS; i++) {
        if (strcmp(lookuptable[i].key, key) == 0)
        {
            return lookuptable[i].val;
        }
    }
    return 100;
}

string get_string(uart_port_t uart_num,u8 cual)
{
	uint8_t ch;
	char dijo[20];
	int son=0,len;
	memset(dijo,0,20);
	while(1)
	{
		len = uart_read_bytes(UART_NUM_0, (uint8_t*)&ch, 1,4/ portTICK_RATE_MS);
		if(len>0)
		{
			if(ch==cual)
				return string(dijo);

			else
				dijo[son++]=ch;
			if (son>sizeof(dijo)-1)
				son=sizeof(dijo)-1;
		}

		vTaskDelay(100/portTICK_PERIOD_MS);
	}
}

void kbd(void *arg) {
	int len;
	uart_port_t uart_num = UART_NUM_0 ;
	uint32_t add;
	char lastcmd=10;
	char data[50],textl[30];
	string s1;
	uint16_t errorcode,code1;
	int cualf;
	int *p=0;
    time_t t;
	struct tm timeinfo ;
	struct timeval tv;
	float amps;

	uart_config_t uart_config = {
			.baud_rate = 115200,
			.data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
			.rx_flow_ctrl_thresh = 122,
			.use_ref_tick=false
	};
	uart_param_config(uart_num, &uart_config);
	uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	esp_err_t err= uart_driver_install(uart_num, UART_FIFO_LEN+10 , 2048, 0, NULL, 0);
	if(err!=ESP_OK)
		printf("Error UART Install %d\n",err);

	while(1)
	{
		len = uart_read_bytes(UART_NUM_0, (uint8_t*)data,2,20/ portTICK_RATE_MS);
		if(len>0)
		{
			if(data[0]==10)
				data[0]=lastcmd;
			lastcmd=data[0];
			switch(data[0])
			{
			case 'h':
			case 'H':
        		printf("Kbd Heap %d\n",xPortGetFreeHeapSize());
        		break;
			case 'a':
				time(&t);
				localtime_r(&t, &timeinfo);
			     timeinfo.tm_mday++;
			     tv.tv_sec = mktime(&timeinfo);
			     tv.tv_usec=0;
			     settimeofday(&tv,0);
			     todate=todate-2;
			     printf("New date\n");
			case 'A':
				printf("Test Amps\n");
				while(gpio_get_level((gpio_num_t)0))
				{
					amps=calcIrms(100,0.0,1110);
					printf("AMPS %.02f\n",amps);
					vTaskDelay(2000 / portTICK_PERIOD_MS);
				}
				printf("Done\n");
				break;
			case 'b':
			case 'B':
					erase_all_timers();
					 printf("ALl Timers erased\n");
				break;

			case 'x':
			case 'X':
				xTaskCreate(&set_FirmUpdateCmd,"firmWMgr",10240,NULL, MGOS_TASK_PRIORITY, NULL);
				break;
			case 'L':
					fclose(bitacora);
					bitacora = fopen("/spiflash/log.txt", "w");//truncate to 0 len
					fclose(bitacora);
					printf("Log cleared\n");
					break;
			case 'l':
				printf("Log:\n");
				fseek(bitacora,0,SEEK_SET);
				while(1)
				{
					//read date
					add=fread(&t,1,4,bitacora);
					if(add==0)
						break;
					//read code
					add=fread(&errorcode,1,2,bitacora);
					if(add==0)
						break;
					add=fread(&code1,1,2,bitacora);
					if(add==0)
						break;
					add=fread(&textl,1,20,bitacora);
					if(add==0)
						{printf("Could not read text log\n");
						break;
						};

					printf("Date %s|Code %d|%s|Code1 %d|%s\n",makeDateString(t).c_str(),errorcode,logText[errorcode].c_str(),code1,textl);
				}
				break;
			case 'q':
			case 'Q':
					printf("Dump core\n");
					vTaskDelay(1000 / portTICK_PERIOD_MS);

				*p=0;
				printf("Paso badaddress\n");
				break;
			case 'v':
			case 'V':{
				printf("Trace Flags ");
				for (int a=0;a<NKEYS/2;a++)
					if (aqui.traceflag & (1<<a))
					{
						if(a<(NKEYS/2)-1)
							printf("%s-",lookuptable[a].key);
						else
							printf("%s",lookuptable[a].key);
					}
				printf("\nEnter TRACE FLAG:");
				fflush(stdout);
				s1=get_string((uart_port_t)uart_num,10);
				memset(textl,0,10);
				memcpy(textl,s1.c_str(),s1.length());
				for (int a=0;a<s1.length();a++)
					textl[a]=toupper(textl[a]);
				s1=string(textl);
				if(strcmp(s1.c_str(),"NONE")==0)
				{
					aqui.traceflag=0;
					write_to_flash();
					break;
				}
				if(strcmp(s1.c_str(),"ALL")==0)
				{
					aqui.traceflag=0xFFFF;
					write_to_flash();
					break;
				}
				cualf=keyfromstring((char*)s1.c_str());
				if(cualf==100)
				{
					printf("Invalid Debug Option\n");
					break;
				}
				if(cualf>=0 )
				{
					printf("Debug Key Pos %d %s added\n",cualf,s1.c_str());
					aqui.traceflag |= 1<<cualf;
					write_to_flash();
					break;
				}
				else
				{
					cualf=cualf*-1;
					printf("Debug Key Pos %d %s removed\n",cualf-(NKEYS/2),s1.c_str());
					aqui.traceflag ^= 1<<(cualf-(NKEYS/2));
					write_to_flash();
					break;
				}

				}

			case 'T':{
				printf("Temp Sensor #:");
				fflush(stdout);
				do{
					len = uart_read_bytes(UART_NUM_0, (uint8_t*)data, sizeof(data),20);
				} while(len==0);
				add=atoi(data);
				if (add>=numsensors)
				{
					printf("Out of range[0-%d]\n",numsensors-1);
					break;
				}
				for (int a=0;a<8;a++)
					printf("%02x",sensors[add][a]);
				printf(" Temp: %0.1f\n",get_temp(add));
				break;}
			case 'f':{
				show_config( 0, true) ;
				break;}
			case 'k':
				printf("Relay Off\n");
				relay(0);
				break;
			case 'K':
				printf("Relay On\n");
				relay(1);
				break;
			case 'S':
				printf("Pos:");
				fflush(stdout);
				s1=get_string((uart_port_t)uart_num,10);
				len=atoi(s1.c_str());
				printf("SSID:");
				fflush(stdout);
				s1=get_string((uart_port_t)uart_num,10);
				memset((void*)&aqui.ssid[len][0],0,sizeof(aqui.ssid[len]));
				memcpy((void*)&aqui.ssid[len][0],(void*)s1.c_str(),s1.length());//without the newline char
				printf("Password:");
				fflush(stdout);
				s1=get_string((uart_port_t)uart_num,10);
				memset((void*)&aqui.pass[len][0],0,sizeof(aqui.pass[len]));
				memcpy((void*)&aqui.pass[len][0],(void*)s1.c_str(),s1.length());//without the newline char

				curSSID=aqui.lastSSID=0;
				write_to_flash();
				break;
			case 'z':
				printf("SetCurSSID to\n");
				do{
					len = uart_read_bytes(UART_NUM_0, (uint8_t*)data, sizeof(data),20);
				} while(len==0);
				add=atoi(data);
				curSSID=0;
				aqui.lastSSID=add;
				write_to_flash();
				printf("CurSSID reset\n");
				break;
			case 'm':
			case 'M':
					aqui.monitorf=!aqui.monitorf;
					printf("Monitor %s\n",aqui.monitorf?"Y":"N");
					write_to_flash();
					break;
			default:
				printf("No cmd\n");
				break;



			}

		}
		vTaskDelay(300 / portTICK_PERIOD_MS);
	}
}


