/***************************************************************************************************
                                    ExploreEmbedded Copyright Notice    
****************************************************************************************************
 * File:   dtu.cpp
 * Version: 1.0
 * Author: ExploreEmbedded
 * Website: http://www.exploreembedded.com/wiki
 * Description: ESP32  Arduino library for dtu demo .
 

**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <Arduino.h>
#include <system_parameter.h>
#include <parameter_config.h>

system_parameter eg118_system_parameter_dtu_module;

#include <RS485.h>
#include <WiFi.h>
#include <esp_wifi.h>


#include <PubSubClient.h>
#include "dtu.h"
#include <drvdout.h>
#include <adc.h>

extern drvout_class eg118_drvout;
extern adc_class eg118_adc;
#define RELOAD_DOG_PIN 3
#define RX1PIN GPIO_NUM_33
#define TX1PIN GPIO_NUM_32


#define PACKET_SIZE 100
char dtu_packet_buf[PACKET_SIZE];
int packet_index = 0;
char mqtt_sub_scribebuf[100];
WiFiClient tcpclient_serialbridge;  //声明一个客户端对象，用于与服务器进行连接

 extern char text_tcpserverip_stringbuf[TCPSERVER_IPstring_length_MAX];
extern char text_tcpserverport_stringbuf[TCPSERVER_PORTstring_length_MAX];
extern char text_mqttserverip_stringbuf[MQTT_SERVER_IPstring_length_MAX];
extern char text_mqttserveruser_stringbuf[MQTTSERVER_USER_length_MAX];
extern char text_mqttserverpass_stringbuf[MQTTSERVER_PASS_length_MAX];

WiFiClient m100espClient;
PubSubClient m100mqttclient(m100espClient);
bool gv_istcpserverconnected = 0;                               //tcp服务器是否连接上的标识
bool gv_ismqttserverconnected = 0;                              //mqtt服务器是否连接上的标识
static unsigned char atmodule_connect_mode = 0;
static unsigned char dtumodule_device_mode = 0;
volatile SemaphoreHandle_t dtu_packet_readyflag_Semaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t lastIsrAt = 0;
volatile SemaphoreHandle_t timerSemaphore;
hw_timer_t *pactetready_timer = NULL;
volatile uint32_t isrCounter = 0;
 system_parameter system_parameter_eg118_dtu_moudle;

IPAddress gv_tcp_server_hostIP;
//const IPAddress default_tcp_server_hostIP(192,168,117,179);  //欲访问的地址
uint16_t gv_serverPort = 2317;                                  //tcp服务器端口号

static void reload_dog(void) 
{


  pinMode(RELOAD_DOG_PIN, OUTPUT);
  digitalWrite(RELOAD_DOG_PIN, 0);
  delay(10);
  digitalWrite(RELOAD_DOG_PIN, 1);
  delay(25);
  digitalWrite(RELOAD_DOG_PIN, 0);
  delay(10);
}

static void dtu_adjustarray(char arr[], unsigned char size) 
{
  //修正字符串里面的换行符
  for (int j = 0; j < size; j++) {
    if (10 == arr[j]) {
      arr[j] = 0;
    }
  }
}

void ARDUINO_ISR_ATTR packet_ready_timercallback(void) 
{
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  if ((gv_istcpserverconnected) || (gv_ismqttserverconnected)) {
    xSemaphoreGiveFromISR(dtu_packet_readyflag_Semaphore, NULL);
  }
}


void creat_packet_ready_timer(void) 
{
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  pactetready_timer = timerBegin(1000000);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(pactetready_timer, &packet_ready_timercallback);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  //timerAlarmWrite(timer, 1000000, true);
  timerAlarm(pactetready_timer, 500000, true, 0);  //50ms

  
}
void start_packetready_timer(void) 
{
  // 使用定时器0，频率1MHz（1微秒精度）
  pactetready_timer = timerBegin(1000000);

  // 绑定中断处理函数
  timerAttachInterrupt(pactetready_timer, &packet_ready_timercallback);

  timerAlarm(pactetready_timer, 2000000, true, 0);  // 200ms
}


void stop_packetready_timer() 
{
  if (pactetready_timer) {
    // Stop and free timer
    timerEnd(pactetready_timer);
    pactetready_timer = NULL;
  }
}

void datefrom_rs485_to_tcp(void) 
{
  // If Timer has fired
  if (xSemaphoreTake(dtu_packet_readyflag_Semaphore, 0) == pdTRUE) {
    //接收到packet已经好了的信息直接发送给tcp服务器。

    tcpclient_serialbridge.print(dtu_packet_buf);
    //delay(100);
    packet_index = 0;
    memset(dtu_packet_buf, 0, PACKET_SIZE);


  }
}

 void senddatators485(char *outmsg, unsigned char msglength) 
{

  RS485.noReceive();
  RS485.beginTransmission();
  for (int i = 0; i < msglength; i++) 
  {
    RS485.beginTransmission();
    RS485.write(outmsg[i]);
    RS485.endTransmission();
    //delay(5);
  }
}

void datefrom_tcp_to_rs485(void) 
{
  int tcp_availnum;
  char tcpreadbuf[100];
  tcp_availnum = tcpclient_serialbridge.available();
  if(tcp_availnum>100)
  {
    tcp_availnum=100;//调整一次读取tcp的数据的大小，我们限制每次读取的buf的大小，缓存buf就这么大读取太多了 放不下会溢出。
    
  }
  memset(tcpreadbuf, 0, 100);
  if (tcp_availnum) {
    for (int i = 0; i < tcp_availnum; i++) 
    {
      if (i < 100) {
        tcpreadbuf[i] = tcpclient_serialbridge.read();
      }
     // reload_dog();
    }  //for
    if (strlen(tcpreadbuf)) {
      senddatators485(tcpreadbuf, tcp_availnum);
      memset(tcpreadbuf, 0, 100);
    }
  }  //if(tcp_availnum)
}


void xTask_tcpDTUfunc_nocrash(void *xTask2) 
{
  unsigned char tcp_serveripstring_length = 0;
  unsigned char tcpserver_port_length = 0;

  char tcpserveripstring[TCPSERVER_IPstring_length_MAX];
  char tcpportstring[TCPSERVER_PORTstring_length_MAX];


  memset(tcpserveripstring, 0, TCPSERVER_IPstring_length_MAX);
  memset(tcpportstring, 0, TCPSERVER_PORTstring_length_MAX);


  // refreshEEPROMRAM();
  system_parameter_eg118_dtu_moudle.parameter_refreshEEPROMRAM();
  Serial.println("[xTask_tcpDTUfunc_nocrash]   LINE 3415->try to connect tcp server.....\n");

  tcp_serveripstring_length = system_parameter_eg118_dtu_moudle.get_tcpserver_ip_length();
  Serial.printf("[xTask_tcpDTUfunc_nocrash]====line 2136->tcp_serveripstring_length=%d:\n", tcp_serveripstring_length);
  if (tcp_serveripstring_length) 
  {

    system_parameter_eg118_dtu_moudle.get_tcpserver_ip_string(tcpserveripstring, tcp_serveripstring_length);
    //修正字符串里面的换行符
    dtu_adjustarray(tcpserveripstring, TCPSERVER_IPstring_length_MAX);
    Serial.printf("[xTask_tcpDTUfunc_nocrash ]====LINE 2774->tcpserveripstring=%s\n", tcpserveripstring);
    String s = tcpserveripstring;
    gv_tcp_server_hostIP.fromString(s);
  } 

  //tcpserver_port_length = EEPROM_get_tcpserver_port_length();
  tcpserver_port_length = system_parameter_eg118_dtu_moudle.get_tcpserver_port_length();
  if (tcpserver_port_length) {

    system_parameter_eg118_dtu_moudle.get_tcpserver_port__string(tcpportstring, tcpserver_port_length);

    dtu_adjustarray(tcpportstring, TCPSERVER_PORTstring_length_MAX);
    gv_serverPort = atoi(tcpportstring);
  } else {
    //gv_serverPort=gv_defaultserverPort;
  }

  Serial.print("[line 2830]  tcpclient_serialbridge.connect to ");
  Serial.println(gv_tcp_server_hostIP);
  Serial.printf("[line 2832]gv_serverPort->%d \n", gv_serverPort);


  gv_istcpserverconnected = tcpclient_serialbridge.connect(gv_tcp_server_hostIP, gv_serverPort);
  if (gv_istcpserverconnected)  //尝试访问目标地址//gv_tcp_server_hostIP default_tcp_server_hostIP
  {
    Serial.println("line 2838  tcpclient_serialbridge.connect succ");
    //start_packetready_timer();//测试timer是否死机

    creat_packet_ready_timer();
    // Create semaphore to inform us when the timer has fired
    dtu_packet_readyflag_Semaphore = xSemaphoreCreateBinary();


    while (1) {
      datefrom_tcp_to_rs485();
      // reload_dog();
      datefrom_rs485_to_tcp();
     // reload_dog();



    }  // while (1)
  }    //if (gv_istcpserverconnected)
  else {

    Serial.println("[xTask_tcpDTUfunc_nocrash][line 2858]tcpclient_serialbridge.connect fail .  nop    nop");
    while (1) {
      delay(2000);
    }
  }

}  //end  xTask_tcpDTUfunc_nocrash

void m100_mqtt_reconnect(void) 
{
  boolean ret_connect_status = 0;
  //int state = 0;
  // Loop until we're reconnected
  if (!m100mqttclient.connected()) {
    //Serial.print("LINE 2320 m100mqttclient.connected()\n");
    // Create a random client ID
    String clientId = "m100ard_Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    ret_connect_status = m100mqttclient.connect(clientId.c_str(), text_mqttserveruser_stringbuf, text_mqttserverpass_stringbuf, 0, 0, 0, 0, 1);

    //state = m100mqttclient.state();

    //Serial.printf("[ state=%d  \n",state);
    //EEPROM_setMQTTERR(state);


    //连接成功后才能订阅主题；
    if (m100mqttclient.connected()) {
      m100mqttclient.subscribe("mqtt_to_rs485");
    }
  }
  //Serial.printf("LINE 2331  MQTT connection...ok ok ok!!!!");
  gv_ismqttserverconnected = 1;
}  //m100_mqtt_reconnect


void mqttreveivedtudata_callback(char *topic, byte *payload, unsigned int length) 
{
  memset(mqtt_sub_scribebuf, 0, 100);
  if (length < 100) {
    strncpy(mqtt_sub_scribebuf, (char *)payload, length);
  } else {
    strncpy(mqtt_sub_scribebuf, (char *)payload, 100);
  }
 //senddatators485(mqtt_sub_scribebuf, length);

  // DO控制命令处理
  if (strstr(mqtt_sub_scribebuf, "DO1:ON")) {
    eg118_drvout.M100DOU1_ON();
    Serial.println("[MQTT] DO1:ON");
  } else if (strstr(mqtt_sub_scribebuf, "DO1:OFF")) {
    eg118_drvout.M100DOU1_OFF();
    Serial.println("[MQTT] DO1:OFF");
  } else if (strstr(mqtt_sub_scribebuf, "DO2:ON")) {
    eg118_drvout.M100DOU2_ON();
    Serial.println("[MQTT] DO2:ON");
  } else if (strstr(mqtt_sub_scribebuf, "DO2:OFF")) {
    eg118_drvout.M100DOU2_OFF();
    Serial.println("[MQTT] DO2:OFF");
  } else if (strstr(mqtt_sub_scribebuf, "GET:IO")) {
    // 获取IO状态命令
    int adc_value = eg118_adc.get_average_adcvalue();
    float current = eg118_adc.get_ADCchangetocurrent(adc_value);
    char io_status[100];
    snprintf(io_status, 100, "AI:%.2fmA", current);
    m100mqttclient.publish("eg118_io_status", io_status);
    Serial.println("[MQTT] GET:IO response sent");
  } else {
    // 其他数据发送到RS485
    senddatators485(mqtt_sub_scribebuf, length);
  }
}
void xTask_mqttDTUfunc(void *xTask2) 
{
  //int ethernet_status = 0;
  //int m100linkstatus;
  //int m100hardware_ethernet;
  unsigned char mqttSERVERIPstring_length = 0;
  unsigned char mqtt_server_user_string_length = 0;
  unsigned char mqtt_server_pass_string_length = 0;

  memset(text_mqttserverip_stringbuf, 0, MQTTSERVER_IP_string_length_MAX);
  memset(text_mqttserveruser_stringbuf, 0, MQTTSERVER_USER_length_MAX);
  memset(text_mqttserverpass_stringbuf, 0, MQTTSERVER_PASS_length_MAX);
  //refreshEEPROMRAM();
  system_parameter_eg118_dtu_moudle.parameter_refreshEEPROMRAM();

  mqttSERVERIPstring_length = system_parameter_eg118_dtu_moudle.get_mqtt_server_ip_length();

  mqtt_server_user_string_length = system_parameter_eg118_dtu_moudle.get_mqtt_server_user_length();

  mqtt_server_pass_string_length = system_parameter_eg118_dtu_moudle.get_mqtt_server_pass_length();
  if (mqttSERVERIPstring_length) {

    system_parameter_eg118_dtu_moudle.get_mqtt_SERVER_string(text_mqttserverip_stringbuf, mqttSERVERIPstring_length);
  }
  if (mqtt_server_user_string_length) {
    // EEPROM_getmqtt_user_string(text_mqttserveruser_stringbuf, mqtt_server_user_string_length);
    system_parameter_eg118_dtu_moudle.get_mqtt_user_string(text_mqttserveruser_stringbuf, mqtt_server_user_string_length);
  }
  if (mqtt_server_pass_string_length) {
    //EEPROM_getmqtt_pass_string(text_mqttserverpass_stringbuf, mqtt_server_pass_string_length);
    system_parameter_eg118_dtu_moudle.get_mqtt_pass_string(text_mqttserverpass_stringbuf, mqtt_server_pass_string_length);
  }
  //修正字符串里面的换行符
  dtu_adjustarray(text_mqttserverip_stringbuf, MQTT_SERVER_IPstring_length_MAX);
  dtu_adjustarray(text_mqttserveruser_stringbuf, MQTTSERVER_USER_length_MAX);
  dtu_adjustarray(text_mqttserverpass_stringbuf, MQTTSERVER_PASS_length_MAX);

  Serial.println("[xTask_mqttDTUfunc]============LINE 3031->设定mqtt服务器参数");
  //Serial.printf("   mqttSERVERIPstring_length=%d mqtt_server_user_string_length=%d mqtt_server_pass_string_length=%d \n", mqttSERVERIPstring_length, mqtt_server_user_string_length, mqtt_server_pass_string_length);
  Serial.printf("[xTask_mqttDTUfunc]============LINE 3033 ->mqttserver-ip:%s\n", text_mqttserverip_stringbuf);
  Serial.printf("[xTask_mqttDTUfunc]============LINE 3034 ->mqttserver-user:%s\n", text_mqttserveruser_stringbuf);
  Serial.printf("[xTask_mqttDTUfunc]===========LINE 3035 ->mqttserver-pass:%s\n", text_mqttserverpass_stringbuf);

  delay(1000);


  m100mqttclient.setServer(text_mqttserverip_stringbuf, 1883);

  m100mqttclient.setCallback(mqttreveivedtudata_callback);
  m100mqttclient.loop();
  if (!m100mqttclient.connected()) {
    m100_mqtt_reconnect();
    m100mqttclient.loop();
  }

  //char debugbuf[20];
  //int length = 0;
  dtu_packet_readyflag_Semaphore = xSemaphoreCreateBinary();
  creat_packet_ready_timer();  
  // IO状态上报定时器
  static unsigned long last_io_report = 0;
  
  while (1) {
    m100mqttclient.loop();

    // 定时上报IO状态（每5秒）
    if (millis() - last_io_report > 5000) {
      last_io_report = millis();
      if (m100mqttclient.connected()) {
        int adc_value = eg118_adc.get_average_adcvalue();
        float current = eg118_adc.get_ADCchangetocurrent(adc_value);
        char io_status[100];
        snprintf(io_status, 100, "AI:%.2fmA", current);
        m100mqttclient.publish("eg118_io_status", io_status);
      }
    }
    
    if (xSemaphoreTake(dtu_packet_readyflag_Semaphore, 0) == pdTRUE) {
      m100mqttclient.loop();
      if (!m100mqttclient.connected()) {
        m100_mqtt_reconnect();
        m100mqttclient.loop();
        delay(500);
        Serial.println("\n     line_ 3691 huawei_iotmqttclient not connect nothing to do ,only huawei_iotmqttclient.loop();\n");
      }
      if (m100mqttclient.connected()) {
        if (strlen(dtu_packet_buf)) {
          m100mqttclient.publish("rs485_to_mqtt", (char *)dtu_packet_buf);
          packet_index = 0;
          memset(dtu_packet_buf, 0, PACKET_SIZE);
          //delay(1500);
        }
      }
    }
  }  // while (1)
}  //xTask_mqttDTUfunc



void xTask_pactet_rs485datafunc(void *xTask2) 
{

  while (1) {
    int availnum = 0;
    delay(100);  //固定周期性休息一下但时间不太长
    //return
    RS485.receive();
    // delay(1000);
    //Serial.println(" line 2335 xTask_readrs485func PRIO IS 3。\n");
    availnum = RS485.available();
    if (availnum) 
    {

      for (int i = 0; i < availnum; i++) 
      {
        if (packet_index < PACKET_SIZE) 
        {
          dtu_packet_buf[packet_index] = RS485.read();
          packet_index++;
          //reload_dog();
        }

        if (packet_index == PACKET_SIZE) {
          //packet_index = 0;
          if ((gv_istcpserverconnected) || (gv_ismqttserverconnected)) 
          {
            //Serial.println(" line 2404   通知dtu task数据包满了可以发出去了。");
            xSemaphoreGive(dtu_packet_readyflag_Semaphore);
            //xSemaphoreGive(dtu_packet_readyflag_Semaphore);
            delay(50);  //打包好了额外奖励休息一下;

          } 
          else 
          {
              // 数据已经填满了buf，但是却没有tcp连接，只能放弃了，延时等着吧
              delay(3000);
            }
        }


      }  //for
    }    //if(availnum)
    else {
      ////485没有检测到数据接下来的时间大概率也是没有数据，这个读数据的task好好休息一会儿。
      delay(1000);
    }
  }  //while(1)
}



void tcpdtu_task_start(void)
{
  if (DEVICE_MODE_TCP_DTU_ONLY == dtumodule_device_mode) {
    Serial.println("[setup]========================LINE 2493 ->xTaskCreate xTask_tcpDTUfunc_nocrash prio  5");
    
    xTaskCreate(
      xTask_tcpDTUfunc_nocrash,   /* Task function. */
      "xTask_tcpDTUfunc_nocrash", /* String with name of task. */
      4096,                       /* Stack size in bytes. */
      NULL,                       /* Parameter passed as input of the task */
      5,                          /* Priority of the task.(configMAX_PRIORITIES - 1 being the highest, and 0 being the lowest.) */
      NULL);

    Serial.println("[setup]========================LINE 2502 -> xTaskCreate xTask_DTUreadrs485func prio  3");
    //response_newcommandresult("line 2502 [setup]",">xTaskCreate xTask_DTUreadrs485func prio  3");
    xTaskCreate(
      xTask_pactet_rs485datafunc,   /* Task function. */
      "xTask_pactet_rs485datafunc", /* String with name of task. */
      4096,                         /* Stack size in bytes. */
      NULL,                         /* Parameter passed as input of the task */
      3,                            /* Priority of the task.(configMAX_PRIORITIES - 1 being the highest, and 0 being the lowest.) */
      NULL);
  }
}
void mqttdtu_task_start(void)
{
  if (DEVICE_MODE_MQTT_DTU_ONLY == dtumodule_device_mode) {
    
    xTaskCreate(
      xTask_mqttDTUfunc,   /* Task function. */
      "xTask_mqttDTUfunc", /* String with name of task. */
      4096,                /* Stack size in bytes. */
      NULL,                /* Parameter passed as input of the task */
      5,                   /* Priority of the task.(configMAX_PRIORITIES - 1 being the highest, and 0 being the lowest.) */
      NULL);

   
    xTaskCreate(
      xTask_pactet_rs485datafunc,   /* Task function. */
      "xTask_pactet_rs485datafunc", /* String with name of task. */
      4096,                         /* Stack size in bytes. */
      NULL,                         /* Parameter passed as input of the task */
      3,                            /* Priority of the task.(configMAX_PRIORITIES - 1 being the highest, and 0 being the lowest.) */
      NULL);
  }

}
int dtu_class::tcp_dtu_start(void)
{
    int rc=0;
   dtumodule_device_mode = eg118_system_parameter_dtu_module.GetdeviceMode();
    tcpdtu_task_start();
    return rc;
}

int dtu_class::mqtt_dtu_start(void)
{
    int rc=0;
   dtumodule_device_mode = eg118_system_parameter_dtu_module.GetdeviceMode();
    mqttdtu_task_start();
    return rc;
}






