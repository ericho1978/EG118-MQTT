#include <OneButton.h>

// ////////////////////////////////m100  sample  config   begin///////////////////////////////////////////
//eg118工程配置开关
#define PHYTESTSAMPLE_ENABLE
#define WEBserverSAMPLE_ENABLE
#define PCMODBUSTOOLDEMO_ENABLE
//#define TH485_PE_DEMO_ENABLE
//#define AWS_CLOUD_ENABLE
#define HUAWEI_CLOUD_ENABLE
#define USRIO_DEMO_ENABLE
//#define LOG_ENABLE
#define HUAWEI_CLOUD_IOT_WITH_NO_TLS
#define ETH_CLOCK_MODE ETH_CLOCK_GPIO0_IN  // 或根据硬件选择其他模式
#define WIFI_DTU_ENABLE
// ===== 系统参数宏定义 =====
#define WIFI_SSIDstring_length_MAX 32
#define WIFI_PASSWARD_LENGTH_MAX 64
#define BASE_MAC_LENGTH 12

#include "esp_system.h"    // 提供 esp_base_mac_addr_set 等系统函数
#include "esp_mac.h"       // 提供 ESP_MAC_WIFI_STA, ESP_MAC_WIFI_SOFTAP 等宏定义 [citation:5][citation:10]
#include <esp_wifi.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <system_parameter.h>
#include <parameter_config.h>
#include <USR_IO.h>
#include <dtu.h>
#include <huaweicloud.h>
#include <Ethernet.h> 
#include <pctool_modbus_demo.h>
#include <adc.h>
#include <atcommand.h>
#include <drvdout.h>
adc_class eg118_adc;
system_parameter system_parameter_eg118_main_moudle;
atclass at;
drvout_class eg118_drvout;
extern const char *softap_ssid ;
extern const char *softap_password ;

AsyncWebServer AsyncWeb_server_instance(80);

//#ifdef AWS_CLOUD_ENABLE
//#include <AWS_IOT.h>
//  #define CUST_BROKER "awwis0u7xuagf.ats.iot.cn-north-1.amazonaws.com.cn"
//  #define CUST_CLOUD_CLIENT_ID "1234"
//  #define CUST_MQTTPUBLISHTOPIC "hardware_m100"
//  char HOST_ADDRESS[] = CUST_BROKER;
//  char CLIENT_ID[] = CUST_CLOUD_CLIENT_ID;
//  char TOPIC_NAME[] = CUST_MQTTPUBLISHTOPIC;
//  AWS_IOT hornbill;  // AWS_IOT instance
//#endif  //AWS_CLOUD_ENABLE


USR_IO usr_io_instance;
dtu_class  dtu_instance;
huaweicloud_class huaweicloud_instance;
#ifdef TH485_PE_DEMO_ENABLE
#include <ArduinoModbus.h>
#define LH_TH485D_DEVICE_ID 1
#define LH_TH485PE_BAUD 9600
#define ADDRESS_LH_TH485D_HUMIDITY 0
#define ADDRESS_LH_TH485D_TEMPTURE 1
#endif

pctool_modbus_demo_class modbus_demo_instance;


#ifdef PHYTESTSAMPLE_ENABLE
#include <Ethernet.h>
#include <ETH.h>

static bool eth_connected = false;


void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
     #ifdef LOG_ENABLE
      Serial.println("[WiFiEvent]====================LINE 0126->ETH Started");
      #endif
      ETH.setHostname("eg118-ethernet");  //set eth hostname here
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:

      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
     #ifdef LOG_ENABLE

      //Serial.print(ETH.macAddress());
      Serial.println(ETH.macAddress());

      Serial.print("[WiFiEvent]====================LINE 0139->ethernet:IPv4:     localip:");
      Serial.println(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print("[WiFiEvent]====================LINE 0142->FULL_DUPLEX");
      }
      Serial.print(",");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      #endif
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("[WiFiEvent]====================LINE 0150->ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("[WiFiEvent]====================LINE 0154->ETH Stopped");
      eth_connected = false;
      break;
    default:
      
      break;
  }
}
#endif



/////////////////////////////////////////公共的define区域begin///////////////////////////////////////////////////////////////////////////////////////////////////////////
//eg118硬件的配置信息
#define ETH_ADDR 1
#define ETH_POWER_PIN 5
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_IP101
#define RELOAD_DOG_PIN 3
#define RX1PIN GPIO_NUM_33
#define TX1PIN GPIO_NUM_32
#define USRIO_ALL_AI_INFO_BUF_SIZE 300
#define USRIO_AI_INFO_BUF_SIZE 50
#define EG118_AI_INFO_BUF_SIZE 50
#define USRIO_DI_INFO_BUF_SIZE 50
#define USRIO_ALL_DI_INFO_BUF_SIZE 300
/////////////////////////////////////////公共的define区域end///////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////公共变量区begin///////////////////////////////////////////////////////////////////////////////////////////////

OneButton OneButton_buttonreload(36, true);

char gv_mqtt_serverarray[MQTTSERVER_IP_string_length_MAX];
char g_text_slot1_usriotype_stringbuf[50];
char g_text_slot2_usriotype_stringbuf[50];
char g_text_slot3_usriotype_stringbuf[50];
char phy_ip_array[20];
char wifi_ip_array[20];
//String wifi_ipstring;
static unsigned char eg118_wifi_mode ;
static unsigned char eg118_device_mode ;
static unsigned char eg118_connect_mode ;

static unsigned short eg118_slot1_usriotype;
static unsigned short eg118_slot2_usriotype;
static unsigned short eg118_slot3_usriotype;
unsigned char gv_tcpserver_ipaddrstring_length = 0;
static  int reloadclickflag = 0;
char gv_ssidstring[WIFI_SSIDstring_length_MAX];
char gv_passward_string[WIFI_PASSWARD_LENGTH_MAX];
char text_SSIDstringbuf[WIFI_SSIDstring_LENGTH_MAX];
char text_PASSWARD_stringbuf[WIFI_PASSWARDstring_LENGTH_MAX];
char text_HOLD_addressstringbuf[10];
char text_HOLD_valuestringbuf[10];

char EG118_ai_1_info[EG118_AI_INFO_BUF_SIZE];//eg118本身的ai1口状态信息
char EG118_di_1_info[20];//eg118本身的di1口状态信息
char usrio_ai_1_info[USRIO_AI_INFO_BUF_SIZE];
char usrio_ai_2_info[USRIO_AI_INFO_BUF_SIZE];
char usrio_ai_3_info[USRIO_AI_INFO_BUF_SIZE];
char usrio_ai_4_info[USRIO_AI_INFO_BUF_SIZE];
char usrio_ai_info[300];
#define SYSTEMINFOSIZE 350
char system_info[SYSTEMINFOSIZE];

char usrio_di_1_info[USRIO_DI_INFO_BUF_SIZE];
char usrio_di_2_info[USRIO_DI_INFO_BUF_SIZE];
char usrio_di_3_info[USRIO_DI_INFO_BUF_SIZE];
char usrio_di_4_info[USRIO_DI_INFO_BUF_SIZE];
char usrio_di_info[300];

//char huaweiiot_mqtt_sub_scribebuf[100];
char awsmessagebuf[80];

 char text_tcpserverip_stringbuf[TCPSERVER_IPstring_length_MAX];
 char text_modbustcpserverip_stringbuf[MODBUSTCPSERVERstring_length_MAX];
 char text_tcpserverport_stringbuf[TCPSERVER_PORTstring_length_MAX];
 char text_mqttserverip_stringbuf[MQTT_SERVER_IPstring_length_MAX];
 char text_mqttserveruser_stringbuf[MQTTSERVER_USER_length_MAX];
 char text_mqttserverpass_stringbuf[MQTTSERVER_PASS_length_MAX];

/////////////////////////////////////////公共变量区 end///////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////函数原型申明以及其他文件定义的变量，这里申明一下原型   begin///////////////////////////////////////////////////////////////////////////////////////////////
extern IPAddress gv_apGateway;
extern IPAddress gv_apSubNet;

extern void response_newcommandresult(char *pstr_targetcommand, char *str_result);

/////////////////////////////////////////函数原型申明end ///////////////////////////////////////////////////////////////////////////////////////////////

static void adjustarray(char arr[], unsigned char size) 
{
  //修正字符串里面的换行符
  for (int j = 0; j < size; j++) 
  {
    if (10 == arr[j]) 
    {
      arr[j] = 0;
    }
  }
}

static unsigned short usrioname_todevice_id(char*ptrselecttext)
{
  int USR_DEV_id=0;

   if (strstr(ptrselecttext,"USR_IO4040"))
  {
    USR_DEV_id=0x101;
  }

  if (strstr(ptrselecttext,"USR_IO0440"))
  {
    USR_DEV_id=0x102;
  }
  if (strstr(ptrselecttext,"USR_IO0080"))
  {
    USR_DEV_id=0x103;
  }
    if (strstr(ptrselecttext,"USR_IO8000"))
  {
    USR_DEV_id=0x104;
  }
  if (strstr(ptrselecttext,"USR_IO0800"))
  {
    USR_DEV_id=0x105;
  }
  if (strstr(ptrselecttext,"USR_IO0404"))
  {
    USR_DEV_id=0x106;
  }
  return USR_DEV_id;
}

void collect_device_info(char *device_info_p, unsigned char device_info_size) 
{


  switch (eg118_device_mode) 
  {

    case DEVICE_Mode_AT_COMMAND:
      snprintf(device_info_p, device_info_size, "rs485  : AT_COMMAND");
    break;
    case DEVICE_MODE_NORMAL:
      snprintf(device_info_p, device_info_size, "rs485  : NORMAL");
    break;

    case DEVICE_MODE_TCP_DTU_ONLY:
    {
      unsigned char tcp_serveripstring_length = 0;
      unsigned char tcpserver_port_length = 0;
      char tcpserverbuf[TCPSERVER_IPstring_length_MAX];
      char tcpportbuf[TCPSERVER_PORTstring_length_MAX];
      memset(tcpserverbuf, 0, TCPSERVER_IPstring_length_MAX);
      memset(tcpportbuf, 0, TCPSERVER_PORTstring_length_MAX);
      tcp_serveripstring_length = system_parameter_eg118_main_moudle.get_tcpserver_ip_length();
      if (tcp_serveripstring_length>TCPSERVER_IPstring_length_MAX) 
      {
        tcp_serveripstring_length=TCPSERVER_IPstring_length_MAX;
      }
      if (tcp_serveripstring_length) 
      {
        system_parameter_eg118_main_moudle.get_tcpserver_ip_string(tcpserverbuf, tcp_serveripstring_length);
      }
      //修正字符串里面的换行符
      adjustarray(tcpserverbuf, TCPSERVER_IPstring_length_MAX);
      tcpserver_port_length = system_parameter_eg118_main_moudle.get_tcpserver_port_length();
      if (tcpserver_port_length>TCPSERVER_IPstring_length_MAX) 
      {
        tcpserver_port_length=TCPSERVER_PORTstring_length_MAX;
      }
      if (tcpserver_port_length) 
      {
      system_parameter_eg118_main_moudle.get_tcpserver_port__string(tcpportbuf, tcpserver_port_length);
      }
      //修正字符串里面的换行符
      adjustarray(tcpportbuf, TCPSERVER_PORTstring_length_MAX);
      snprintf(device_info_p, device_info_size, "rs485  : TCP_DTU.\n tcpserver : %s\n tcpport   : %s\n", tcpserverbuf, tcpportbuf);
      
    }
    break;


    case DEVICE_MODE_MQTT_DTU_ONLY:
    {
        unsigned char mqtt_serveripstring_length = 0;
        unsigned char mqtt_server_user_string_length = 0;
        unsigned char mqtt_server_pass_string_length = 0;
        char mqttserver[MQTT_SERVER_IPstring_length_MAX];
        char text_mqttserveruser_stringbuf[MQTTSERVER_USER_length_MAX];
        char text_mqttserverpass_stringbuf[MQTTSERVER_PASS_length_MAX];
        memset(mqttserver, 0, MQTT_SERVER_IPstring_length_MAX);
        memset(text_mqttserveruser_stringbuf, 0, MQTTSERVER_USER_length_MAX);
        memset(text_mqttserverpass_stringbuf, 0, MQTTSERVER_PASS_length_MAX);
      mqtt_serveripstring_length = system_parameter_eg118_main_moudle.get_mqtt_server_ip_length();
      if (mqtt_serveripstring_length>MQTT_SERVER_IPstring_length_MAX)
      {
        mqtt_serveripstring_length=MQTT_SERVER_IPstring_length_MAX;
      }
      if (mqtt_serveripstring_length) 
      {
        system_parameter_eg118_main_moudle.get_mqtt_SERVER_string(mqttserver, mqtt_serveripstring_length);
      }
      //修正字符串里面的换行符
      adjustarray(mqttserver, MQTT_SERVER_IPstring_length_MAX);

      mqtt_server_user_string_length = system_parameter_eg118_main_moudle.get_mqtt_server_user_length();
      if (mqtt_server_user_string_length>MQTTSERVER_USER_length_MAX)
      {
        mqtt_server_user_string_length=MQTTSERVER_USER_length_MAX;
      }
      if (mqtt_server_user_string_length) 
      {
        system_parameter_eg118_main_moudle.get_mqtt_user_string(text_mqttserveruser_stringbuf, mqtt_server_user_string_length);
      }
      mqtt_server_pass_string_length = system_parameter_eg118_main_moudle.get_mqtt_server_pass_length();
      if (mqtt_server_pass_string_length>MQTTSERVER_PASS_length_MAX)
      {
        mqtt_server_pass_string_length=MQTTSERVER_PASS_length_MAX;
      }
      if (mqtt_server_pass_string_length) 
      {
        system_parameter_eg118_main_moudle.get_mqtt_pass_string(text_mqttserverpass_stringbuf, mqtt_server_pass_string_length);
      }
      snprintf(device_info_p, device_info_size, "  rs485  :MQTT_DTU.\n    server: %s\n    user  : %s\n    pass  : %s>\n", mqttserver, text_mqttserveruser_stringbuf, text_mqttserverpass_stringbuf);
    }
    break;
    case DEVICE_MODE_MODBUS_RTU:
        {
            int slavedeviceid=0;
            int address=0;
            slavedeviceid=modbus_demo_instance.getSetupSlaveid();
            address=modbus_demo_instance.getSetupAddress();
            snprintf(device_info_p, device_info_size, "   rs485 Mode  : MODBUS_RTU\n      deviceid : %d\n      address  : %d\n         BAUD  : 115200",slavedeviceid,address);
        }
    break;
    case DEVICE_MODE_MODBUS_TCP:
    {
      unsigned char modbustcp_serverstring_length = 0;
      char text_modbustcpserver_stringbuf[MODBUSTCPSERVERstring_length_MAX];
      modbustcp_serverstring_length = system_parameter_eg118_main_moudle.get_modbusserver_ip_length();
      if(modbustcp_serverstring_length>MODBUSTCPSERVERstring_length_MAX)
      {
        modbustcp_serverstring_length=MODBUSTCPSERVERstring_length_MAX;
      }
      system_parameter_eg118_main_moudle.get_modbusserver_ip_string(text_modbustcpserver_stringbuf,modbustcp_serverstring_length);
      adjustarray(text_modbustcpserver_stringbuf, modbustcp_serverstring_length);
      snprintf(device_info_p, device_info_size, "rs485 Mode :MODBUS_TCP\n modbustcpserver:%s modbustcp_serverstring_length=%d",text_modbustcpserver_stringbuf,modbustcp_serverstring_length);
    }
    break;

    default:
      snprintf(device_info_p, device_info_size, "device mode%d is  unknow",eg118_device_mode);
    break;
  }
}  //collect_device_info



void collect_wifi_info(char *p_wifi_info_array, unsigned char wifi_info_array_size) 
{
  switch (eg118_wifi_mode) 
  {
    case EEPROM_WIFI_AP_MODE:
      snprintf(p_wifi_info_array, wifi_info_array_size, "ap ip is 192.168.1.1");
    break;
    case EEPROM_WIFI_STA_MODE:
      snprintf(p_wifi_info_array, wifi_info_array_size, " sta ssid     : %s\n  sta password : %s \n", gv_ssidstring, gv_passward_string);
    break;
    default:
      snprintf(p_wifi_info_array, wifi_info_array_size, " m100 wifi mode is unknow ");
    break;
  }
}

void collect_usrio_info(char *slotusrio_info_p, unsigned char slotusrio_info_size) 
{
  unsigned short slot1_usriotype=0;
  unsigned short slot2_usriotype=0;
  unsigned short slot3_usriotype=0;


  char slot1_usrioname[25];
  char slot2_usrioname[25];
  char slot3_usrioname[25];

    slot1_usriotype=system_parameter_eg118_main_moudle.get_slot1_usrio_type();
    slot2_usriotype=system_parameter_eg118_main_moudle.get_slot2_usrio_type();
    slot3_usriotype=system_parameter_eg118_main_moudle.get_slot3_usrio_type();
  
  switch (slot1_usriotype)
  {
    case 0x101:
      snprintf(slot1_usrioname,25,"%s","USR_IO4040[4DI,4DO]");
    break;
    case 0x102:
      snprintf(slot1_usrioname,25,"%s","USR_IO0440[4AI,4DO]");
    break;
     case 0x103:
      snprintf(slot1_usrioname,25,"%s","USR_IO0080[8DO]");
    break;
    case 0x1004:
    snprintf(slot1_usrioname,25,"%s","USR_IO8000[8DI]");
    break;
    case 0x105:
      snprintf(slot1_usrioname,25,"%s","USR_IO0800[8AI]");
    break;
    case 0x106:
      snprintf(slot1_usrioname,25,"%s","USR_IO0404[4AI+4AO]");
    break;
    default:
     snprintf(slot1_usrioname,25,"slot1 type %x ",slot1_usriotype);
    break;
  }
    switch (slot2_usriotype)
  {
    case 0x101:
    
    snprintf(slot2_usrioname,25,"%s","USR_IO4040[4DI,4DO]");
    break;
    case 0x102:
   
    snprintf(slot2_usrioname,25,"%s","USR_IO0440[4AI,4DO]");
    break;
     case 0x103:
    
    snprintf(slot2_usrioname,25,"%s","USR_IO0080[8DO]");
    break;
    case 0x1004:
      snprintf(slot2_usrioname,25,"%s","USR_IO8000[8DI]");
    break;
    case 0x105:
    snprintf(slot2_usrioname,25,"%s","USR_IO0800[8AI]");
    break;
    case 0x106:
      snprintf(slot2_usrioname,25,"%s","USR_IO0404[4AI+4AO]");
    break;
    default:
      snprintf(slot2_usrioname,25,"slot2 type %x ",slot2_usriotype);
     break;
  }
  switch (slot3_usriotype)
  {
    case 0x101:
      snprintf(slot3_usrioname,25,"%s","USR_IO4040[4DI,4DO]");
    break;
    case 0x102:
    snprintf(slot3_usrioname,25,"%s","USR_IO0440[4AI,4DO]");
    break;
    case 0x103:
      snprintf(slot3_usrioname,25,"%s","USR_IO0080[8DO]");
    break;
    case 0x1004:
      snprintf(slot3_usrioname,25,"%s","USR_IO8000[8DI]");
    break;
    case 0x105:
      snprintf(slot3_usrioname,25,"%s","USR_IO0800[8AI]");
    break;
    case 0x106:
     snprintf(slot3_usrioname,25,"%s","USR_IO0404[4AI+4AO]");
    break;
    default:
      snprintf(slot3_usrioname,25,"slot3 type %x ",slot3_usriotype);
     break;
  }
  snprintf(slotusrio_info_p, slotusrio_info_size, "usriotype:\n slot1  : %s\n slot2  : %s\n slot3  : %s\n", slot1_usrioname, slot2_usrioname,slot3_usrioname);
}
void collect_device_factory_info(char *device_factory_info_p, unsigned char device_factory_size) 
{
  unsigned char eg118SNlength = 0;
  unsigned char m100muverlength = 0;
  char eg118snarray[M100_SN_LENGTH_MAX];
  char pruductiondate[M100_MUVER_LENGTH_MAX];
  memset(eg118snarray, 0, M100_SN_LENGTH_MAX);
  memset(pruductiondate, 0, M100_MUVER_LENGTH_MAX);

  eg118SNlength = system_parameter_eg118_main_moudle.get_sn_length();
  if (eg118SNlength>M100_SN_LENGTH_MAX) 
  {
    eg118SNlength=M100_SN_LENGTH_MAX;
  }
  if (eg118SNlength) 
  {
    system_parameter_eg118_main_moudle.getm100sn_string(eg118snarray, eg118SNlength);
    //修正字符串里面的换行符
    adjustarray(eg118snarray, eg118SNlength);
  } else 
  {
    strcpy(eg118snarray, "sn is NULL");
  }

  m100muverlength = system_parameter_eg118_main_moudle.getM100_MUVER_length();
  if (m100muverlength>M100_MUVER_LENGTH_MAX)
  {
    m100muverlength=M100_MUVER_LENGTH_MAX;
  }
if (m100muverlength) 
{
   
    system_parameter_eg118_main_moudle.getM100_MUVER_string(pruductiondate, m100muverlength);
    //修正字符串里面的换行符
    adjustarray(pruductiondate, m100muverlength);
} 
else 
{
    strcpy(pruductiondate, __DATE__);
}

  snprintf(device_factory_info_p, device_factory_size, "sn            : %s\n pruductiondate: %s\n", eg118snarray, pruductiondate);
}
#define DEVICE_INFO_BUF_SIZE 90
#define USRIOINFOBUF_SIZE 110
char g_usriotype_info[USRIOINFOBUF_SIZE];
void collect_system_info(void) 
{
  char wifi_info[50];
 // char usriotype_info[80];//定义成局部变量，异常了，难道栈空间不够了。这里就改成全局的。
  char device_info[DEVICE_INFO_BUF_SIZE];
  char device_factory_info[80];
  memset(g_usriotype_info, 0, USRIOINFOBUF_SIZE);
  memset(device_factory_info, 0, 80);
  memset(device_info, 0, DEVICE_INFO_BUF_SIZE);
  memset(wifi_info, 0, 50);
   collect_usrio_info(g_usriotype_info, USRIOINFOBUF_SIZE);
  collect_device_factory_info(device_factory_info, 80);
  collect_device_info(device_info, DEVICE_INFO_BUF_SIZE);
  collect_wifi_info(wifi_info, 50);
  memset(system_info, 0, SYSTEMINFOSIZE);
  snprintf(system_info, SYSTEMINFOSIZE, "USR-EG118 system infomation %s\n %s\n%s\n %s\n",g_usriotype_info,device_factory_info, device_info, wifi_info);
}

void handleRoot() {
  //digitalWrite(led, 1);
  //server.send(200, "text/plain", "hello from esp32!");

  printconfigweb();
  //digitalWrite(led, 0);
}

void handleNotFound() {

  //digitalWrite(led, 0);
}
void printconfigweb(void) {
}
char configmsg[200];


void callbackset_0_m100rootweb(void) {
  AsyncWeb_server_instance.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html",
                  "<html>\
      <head>\
        <title>EG118 rootweb   </title>\
      </head>\
      <body>\
        <h1>USR-EG118:   EG118 rootweb   </h1>\
        <form>\
            <input type='button' value='1 EG118_config' ' onclick='document.location=\"/PAGE_EG118_CONFIG\"'>\
            <input type='button' value='2 EG118_demo' ' onclick='document.location=\"/PAGE_EG118_DEMO\"'>\
            <input type='button' value='3 EG118_rs485_WORKFUNC' ' onclick='document.location=\"/PAGE_RS485_WORKFUNC\"'>\
            <input type='button' value='4 EG118_device_info' ' onclick='document.location=\"/PAGE_SYSTEM_INFO\"'>\
        </form>\
      </body>\
    </html>");
  });
}




void callbakset_0_m100_reback_to_root_web(void) {
  AsyncWeb_server_instance.on("/PAGE_reback_to_root_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html",
                  "<html>\
      <head>\
        <title>USR-EG118 rootweb   </title>\
      </head>\
      <body>\
        <h1>USR-EG118:1.set eg118config  2.eg118 demo  3. rs485_WORKFUNC </h1>\
        <form>\
            <input type='button' value='1 EG118_config' ' onclick='document.location=\"/PAGE_EG118_CONFIG\"'>\
            <input type='button' value='2 EG118_demo' '   onclick='document.location=\"/PAGE_EG118_DEMO\"'>\
            <input type='button' value='3 EG118_rs485_workfunc' ' onclick='document.location=\"/PAGE_RS485_WORKFUNC\"'>\
            <input type='button' value='4 EG118_device_info'    ' onclick='document.location=\"/PAGE_SYSTEM_INFO\"'>\ 
        </form>\
      </body>\
    </html>");
  });
}
void save_sta_wifi_info_toeeprom(char* ptr_wifissidstring,char* ptr_wifipasswordstring) 
{
  unsigned char wifissid_length = 0;
  unsigned char wifipassword_length = 0;
  wifissid_length = strlen(ptr_wifissidstring);
  wifipassword_length=strlen(ptr_wifipasswordstring);

  if(wifissid_length>WIFI_SSIDstring_length_MAX)
  {
    wifissid_length=WIFI_SSIDstring_length_MAX;
  }
  if (wifipassword_length> WIFI_PASSWARD_LENGTH_MAX) 
  {
      wifipassword_length=WIFI_PASSWARD_LENGTH_MAX;
  }
  //tcp连接是一对的，必须同时要有ip地址与端口号
  if ( (wifissid_length) &&(wifipassword_length) )
  {
    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.set_wifissid_length(wifissid_length);
    system_parameter_eg118_main_moudle.setpoweronwifiSSIDstr(ptr_wifissidstring, wifissid_length);

    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.setwifi_password_length(wifipassword_length);
    system_parameter_eg118_main_moudle.setwifi_password_str(ptr_wifipasswordstring, wifipassword_length);

  }
}

void savetcpserverinfo_toeeprom(char* ptr_tcpserverstring,char* ptr_tcportstring) 
{
  unsigned char tcpserver_ip_length = 0;
  unsigned char tcpserver_port_length = 0;
  tcpserver_ip_length = strlen(ptr_tcpserverstring);
  tcpserver_port_length=strlen(ptr_tcportstring);

  if(tcpserver_ip_length>TCPSERVER_IPstring_length_MAX)
  {
    tcpserver_ip_length=TCPSERVER_IPstring_length_MAX;
  }
  if (tcpserver_port_length> TCPSERVER_PORTstring_length_MAX) 
  {
      tcpserver_port_length=TCPSERVER_PORTstring_length_MAX;
  }
  //tcp连接是一对的，必须同时要有ip地址与端口号
  if ( (tcpserver_ip_length) &&(tcpserver_port_length) )
  {
    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.set_tcpserver_ip_length(tcpserver_ip_length);
    system_parameter_eg118_main_moudle.set_tcpserver_ip_string(ptr_tcpserverstring, tcpserver_ip_length);

    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.set_tcpserver_port_length(tcpserver_port_length);
    system_parameter_eg118_main_moudle.set_tcpserver_port__string(ptr_tcportstring, tcpserver_port_length);

  }
}


void savemqttserverinfo_toeeprom(char* ptr_mqttserverstring,char* ptr_mqtt_userstring,char* ptr_mqtt_passwordstring) 
{
  unsigned char mqttserver_ip_length = 0;
  unsigned char mqttserver_user_length = 0;
  unsigned char mqttserver_password_length = 0;
  mqttserver_ip_length = strlen(ptr_mqttserverstring);
  mqttserver_user_length=strlen(ptr_mqtt_userstring);
   mqttserver_password_length=strlen(ptr_mqtt_passwordstring);

  if(mqttserver_ip_length>MQTT_SERVER_IPstring_length_MAX)
  {
    mqttserver_ip_length=MQTT_SERVER_IPstring_length_MAX;
  }
  if (mqttserver_user_length> MQTTSERVER_USER_string_length_MAX) 
  {
      mqttserver_user_length=MQTTSERVER_USER_string_length_MAX;
  }
    if (mqttserver_password_length> MQTTSERVER_PASSWORD_string_length_MAX) 
  {
      mqttserver_password_length=MQTTSERVER_PASSWORD_string_length_MAX;
  }
  //设定用户设置mqtt接入的信息不回为空，如果空的应该不对不写入；
  if ( (mqttserver_ip_length) &&(mqttserver_user_length)&&(mqttserver_password_length) )
  {
    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.set_mqtt_server_ip_length(mqttserver_ip_length);
    system_parameter_eg118_main_moudle.set_mqttserver_ip_string(ptr_mqttserverstring, mqttserver_ip_length);

    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.set_mqtt_server_user_length(mqttserver_user_length);
    system_parameter_eg118_main_moudle.set_mqttserver_user_string(ptr_mqtt_userstring, mqttserver_user_length);

    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.set_mqtt_server_pass_length(mqttserver_password_length);
    system_parameter_eg118_main_moudle.set_mqtt_pass_string(ptr_mqtt_passwordstring, mqttserver_password_length);

  }
}



void save_modbustcpserverinfo_toeeprom(char* ptr_modbus_tcpserver_ipstring) 
{
  unsigned char modbus_tcpserver_ip_length = 0;
  
  modbus_tcpserver_ip_length = strlen(ptr_modbus_tcpserver_ipstring);
  

  if(modbus_tcpserver_ip_length>MODBUSTCPSERVERstring_length_MAX)
  {
    modbus_tcpserver_ip_length=MODBUSTCPSERVERstring_length_MAX;
  }

  if  (modbus_tcpserver_ip_length )
  {
    system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
    system_parameter_eg118_main_moudle.set_modbustcpserver_ip_length(modbus_tcpserver_ip_length);
    system_parameter_eg118_main_moudle.set_modbustcpserver_ip_string(ptr_modbus_tcpserver_ipstring, modbus_tcpserver_ip_length);

  }
}


//原始的wifi设定的代码，

void callbackset_3_3_1_tcpserversettingweb(void) 
{
  AsyncWeb_server_instance.on("/tcpserversettingPAGE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "\
    <html><body>\
    <form action=\"/process\">\
    </div>\
    <div class=\"row form-group\">\
        <label  class=\"col-xs-3 side-right\" for=\"tcpserver_ip\">tcpserverip:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='tcpserver_ip' name='tcpserver_ip'>\
     </div>\
    <div class=\"row form-group\">\
    <label  class=\"col-xs-3 side-right\" for=\"tcpserver_port\">tcpserverport:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='tcpserver_port' name='tcpserver_port'>\
    </div>\
    </div>\
    <input type='submit'value='submit'>\
    <input type='reset'value='reset'>\
    </form>\
    </body>\
    </html>");
  });
  AsyncWeb_server_instance.on("/process", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("tcpserver_ip")) {
      String tcpserver_ipmessage;

      tcpserver_ipmessage = request->getParam("tcpserver_ip")->value();
      tcpserver_ipmessage.toCharArray(text_tcpserverip_stringbuf, 50, 0);
     savetcpserverinfo_toeeprom(text_tcpserverip_stringbuf,text_tcpserverport_stringbuf);
    }
  });

}  //callbackset_4_tcpserversettingweb

#ifdef PCMODBUSTOOLDEMO_ENABLE
void callbackset_2_4_modbus_slavedevice_setting_web(void) {
  AsyncWeb_server_instance.on("/PAGE_RS485_MODBUS_SLAVESETTING", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "\
    <html><body>\
    <form action=\"/process\">\
        </div>\
    <div class=\"row form-group\">\
        <label  class=\"col-xs-3 side-right\" for=\"holdregisteraddress\">holdregisteraddress:</label>\ 
    <div class=\"col-xs-6\">\
<input type='text' id='holdregisteraddress'name='holdregisteraddress'>\
    <div class=\"row form-group\">\
    <label  class=\"col-xs-3 side-right\" for=\"holdregister_writevalue\">holdregisterwritevalue:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='holdregister_writevalue' name='holdregister_writevalue'>\
    </div>\
    </div>\
    <input type='submit'value='submit'>\
    <input type='reset'value='reset'>\
    </form>\
    </body>\
    </html>");
  });
  AsyncWeb_server_instance.on("/process", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("holdregisteraddress")) {
      String holdregisteraddressmessage;
      int v_holdregisteraddress = 0;
      
      //String passwardmessage;
      holdregisteraddressmessage = request->getParam("holdregisteraddress")->value();
      holdregisteraddressmessage.toCharArray(text_HOLD_addressstringbuf, 10, 0);


      v_holdregisteraddress = atoi(text_HOLD_addressstringbuf);
      modbus_demo_instance.setholdregisteraddress(v_holdregisteraddress);


    }

    if (request->hasParam("holdregister_writevalue")) {
      String holdregister_writevaluemessage;
      int v_holdregistervalue = 0;
      holdregister_writevaluemessage = request->getParam("holdregister_writevalue")->value();
      holdregister_writevaluemessage.toCharArray(text_HOLD_valuestringbuf, 10, 0);
      v_holdregistervalue = atoi(text_HOLD_valuestringbuf);
      modbus_demo_instance.setholdregistervalue(v_holdregistervalue);
    }
  });


}  //callbackset_8_modbus_writehold_value_web

void callbackset_3_4_1_modbus_tcpserverip_setting_web(void) {
  AsyncWeb_server_instance.on("/PAGE_RS485_MODBUS_TCP_SERVERSETTING", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "\
    <html><body>\
    <form action=\"/process\">\
        </div>\
    <div class=\"row form-group\">\
        <label  class=\"col-xs-3 side-right\" for=\"holdregisteraddress\">holdregisteraddress:</label>\ 
    <div class=\"col-xs-6\">\
<input type='text' id='holdregisteraddress'name='holdregisteraddress'>\
    <div class=\"row form-group\">\
    <label  class=\"col-xs-3 side-right\" for=\"holdregister_writevalue\">holdregisterwritevalue:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='holdregister_writevalue' name='holdregister_writevalue'>\
    </div>\
    </div>\
    <input type='submit'value='submit'>\
    <input type='reset'value='reset'>\
    </form>\
    </body>\
    </html>");
  });
  AsyncWeb_server_instance.on("/process", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("holdregisteraddress")) 
    {
      String holdregisteraddressmessage;
      int v_holdregisteraddress = 0;
      //String passwardmessage;
      holdregisteraddressmessage = request->getParam("holdregisteraddress")->value();
      holdregisteraddressmessage.toCharArray(text_HOLD_addressstringbuf, 10, 0);
      v_holdregisteraddress = atoi(text_HOLD_addressstringbuf);
      modbus_demo_instance.setholdregisteraddress(v_holdregisteraddress);
    }

    if (request->hasParam("holdregister_writevalue")) 
    {
      int v_holdregistervalue = 0;
      String holdregister_writevaluemessage;
      holdregister_writevaluemessage = request->getParam("holdregister_writevalue")->value();
      holdregister_writevaluemessage.toCharArray(text_HOLD_valuestringbuf, 10, 0);
      v_holdregistervalue = atoi(text_HOLD_valuestringbuf);
      modbus_demo_instance.setholdregistervalue(v_holdregistervalue);
    }
  });


}  //callbackset_8_modbus_writehold_value_web
void callbackset2_3_2_1_modbustcpserversettingweb(void) 
{
  AsyncWeb_server_instance.on("/PAGE_RS485_MODBUS_TCP_SERVERSETTING", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "\
    <html><body>\
    <form action=\"/process\">\
    </div>\
    <div class=\"row form-group\">\
        <label  class=\"col-xs-3 side-right\" for=\"modbustcpserver_ip\">modbustcpserver_ip:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='modbustcpserver_ip' name='modbustcpserver_ip'>\
     </div>\
    <input type='submit'value='submit'>\
    <input type='reset'value='reset'>\
    </form>\
    </body>\
    </html>");
  });
  AsyncWeb_server_instance.on("/process", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("modbustcpserver_ip")) 
    {
      String modbustcpserver_ipmessage;
      modbustcpserver_ipmessage = request->getParam("modbustcpserver_ip")->value();
      modbustcpserver_ipmessage.toCharArray(text_modbustcpserverip_stringbuf, MODBUSTCPSERVERstring_length_MAX, 0);
     save_modbustcpserverinfo_toeeprom(text_modbustcpserverip_stringbuf);
    }
  });

}  //callbackset_4_tcpserversettingweb



#endif



void callbackset_3_2_1_mqttserversettingweb(void) {
  AsyncWeb_server_instance.on("/mqttserversettingPAGE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "\
    <html><body>\
    <form action=\"/process\">\
    </div>\
    <div class=\"row form-group\">\
        <label  class=\"col-xs-3 side-right\" for=\"mqttserver_ip\">mqttserverip:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='mqttserver_ip' name='mqttserver_ip'>\
     </div>\
    <div class=\"row form-group\">\
    <label  class=\"col-xs-3 side-right\" for=\"mqttserver_user\">mqttserveruser:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='mqttserver_user' name='mqttserver_user'>\
    </div>\
    <div class=\"row form-group\">\
        <label  class=\"col-xs-3 side-right\" for=\"mqttserver_pass\">mqttserverpass:</label>\ 
    <div class=\"col-xs-6\">\
    <input type='text' id='mqttserver_pass' name='mqttserver_pass'>\
     </div>\
    </div>\
    <input type='submit'value='submit'>\
    <input type='reset'value='reset'>\
    </form>\
    </body>\
    </html>");
  });
  AsyncWeb_server_instance.on("/process", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("mqttserver_ip")) {
      String mqttserverip_message;

      mqttserverip_message = request->getParam("mqttserver_ip")->value();
      mqttserverip_message.toCharArray(text_mqttserverip_stringbuf, 50, 0);
      savemqttserverinfo_toeeprom(text_mqttserverip_stringbuf,text_mqttserveruser_stringbuf,text_mqttserverpass_stringbuf);
    }
    if (request->hasParam("mqttserver_user")) {
      String mqttserver_usermessage;
      mqttserver_usermessage = request->getParam("mqttserver_user")->value();
      mqttserver_usermessage.toCharArray(text_mqttserveruser_stringbuf, MQTTSERVER_USER_length_MAX, 0);
     savemqttserverinfo_toeeprom(text_mqttserverip_stringbuf,text_mqttserveruser_stringbuf,text_mqttserverpass_stringbuf);
    }
    if (request->hasParam("mqttserver_pass")) {
      String mqttserver_passmessage;
      mqttserver_passmessage = request->getParam("mqttserver_pass")->value();
      mqttserver_passmessage.toCharArray(text_mqttserverpass_stringbuf, MQTTSERVER_PASS_length_MAX, 0);
      savemqttserverinfo_toeeprom(text_mqttserverip_stringbuf,text_mqttserveruser_stringbuf,text_mqttserverpass_stringbuf);
    }
  });
}  //callbackset_5_1_mqttserversettingweb



/////////////////////////////////////begin/////////////////////////////////////////

///////////////////////////////end/////////////////////////////////////////////////////////

void callbackset_1_2_doutconfig_web(void) {
  AsyncWeb_server_instance.on("/PAGE_doutconfig_ONBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
              <head>\
                <title>USR-EG118 DOUT SET   succ   </title>\
              </head>\
              <body>\
                <h1>USR-EG118:  1set DOUT0 ON OFF DOUT1 ON OFF get ai current   </h1>\
                <form>\
                    <input type='button' value='1 OUT1_ON ' onclick='document.location=\"/PAGE_OUT1_ONBUTTON\"'>\
                    <input type='button' value='2 OUT1_OFF ' onclick='document.location=\"/PAGE_OUT1_OFFBUTTON\"'>\
                    <input type='button' value='3 OUT2_ON ' onclick='document.location=\"/PAGE_OUT2_ONBUTTON\"'>\
                    <input type='button' value='4 OUT_2OFF' onclick='document.location=\"/PAGE_OUT2_OFFBUTTON\"'>\
                    <input type='button' value='5 eg118_ai' onclick='document.location=\"/PAGE_GET_EG118_AI_BUTTON\"'>\
                    <input type='button' value='6 eg118_di' onclick='document.location=\"/PAGE_get_EG118_DI_BUTTON\"'>\
                    <input type='button' value='return' ' onclick='document.location=\"/PAGE_EG118_CONFIG\"'>\
                </form>\
              </body>\
            </html>");
  });
}

void callbackset_1_3_softapmode_web(void)
{
  AsyncWeb_server_instance.on("/PAGE_wifi_SOFTAP_modeBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>DOUT  set succed  </title>\
      </head>\
      <body>\
        <h1>:  SET USR-EG118 to SOFTAP MODE when USR-EG118 POWER ON </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");

    system_parameter_eg118_main_moudle.set_wifi_status(EEPROM_WIFI_AP_MODE);
  });

}  //end callbackset_3_SOFTAP_MODEweb

void callbackset_1_4_connecttonetmode_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_CONNECT_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_SET_CONNECT_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>:  select USR-EG118 connect to net over wifi or ethernet  when USR-EG118 POWER ON </h1>\
        <form>\
            <input type='button' value='wifi ' ' onclick='document.location=\"/PAGE_CONNECT_MODE_WIFI_INFO\"'>\
            <input type='button' value='ethernet' ' onclick='document.location=\"/PAGE_CONNECT_MODE_ETHERNET_INFO\"'>\
        </form>\
      </body>\
    </html>");
  });
}


void callbackset_3_1_at_mode_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_RS485AT_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>MQTT_DTU_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>USR-EG118:  SET USR-EG118 work satcommand  MODE  when USR-EG118 POWER ON </h1>\
      </body>\
    </html>");
    //system_parameter_eg118_main_moudle.set_wifi_status(WIFI_AP);

    system_parameter_eg118_main_moudle.set_deviceMode_AT_MODE();
  });
}





void callbackset_1_2_1_DOUT1_ONsuccweb(void) {
  //unsigned char out2status=0;
  AsyncWeb_server_instance.on("/PAGE_OUT1_ONBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>DOUT1 on   set succed  </title>\
      </head>\
      <body>\
        <h1>:    dout1  on  press return BUTTON to reback PAGE_doutconfig_ONBUTTON </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_doutconfig_ONBUTTON\"'>\
        </form>\
      </body>\
    </html>");
    eg118_drvout.M100DOU1_ON();
  });

}  //end callbackset_1_1_9_DOUT1_ONsuccweb)

void callbackset_1_2_2_DOUT1_OFFsuccweb(void) 
{

  AsyncWeb_server_instance.on("/PAGE_OUT1_OFFBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>DOUT  set succed  </title>\
      </head>\
      <body>\
        <h1>:  dout1  off. press return BUTTON to reback PAGE_doutconfig_ONBUTTON </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_doutconfig_ONBUTTON\"'>\
        </form>\
      </body>\
    </html>");
    eg118_drvout.M100DOU1_OFF();
  });
}  //end callbackset_1_2_9_DOUT1_OFFsuccweb





void callbackset_2_2_2_2_1AO1_OUTPUTCURRENT_4ma_succweb(void) {
  //unsigned char out2status=0;
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTCURRENT_4ma_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 4ma current  set succed  </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out is 4 ma current </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");

    // out2status=EEPROM_GET_OUT2STATUS();
   usr_io_instance.ao_current_set(1, 1, 4);
  });

}  //end callbackset_14_1_AO1_OUTPUTCURRENT_4ma_succweb
void callbackset_2_2_2_2_2AO1_OUTPUTCURRENT_5ma_succweb(void) {
  //unsigned char out2status=0;
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTCURRENT_5ma_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 5 mA current    </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out is 5mA current </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
    usr_io_instance.ao_current_set(1, 1, 5);
  });

}  //end callbackset_14_1_AO1_OUTPUTCURRENT_4ma_succweb
void callbackset_2_2_2_2_3AO1_OUTPUTCURRENT_10ma_succweb(void) {
  //unsigned char out2status=0;
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTCURRENT_10ma_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 10ma current  set succed  </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out is 10 mA current </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
    usr_io_instance.ao_current_set(1, 1, 10);

  });

}  //end callbackset_14_1_AO1_OUTPUTCURRENT_4ma_succweb
void callbackset_2_2_2_2_4AO1_OUTPUTCURRENT_20ma_succweb(void) {
  //unsigned char out2status=0;
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTCURRENT_20ma_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 20ma current  set succed  </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out  is 20 mA current   </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
     usr_io_instance.ao_current_set(1, 1, 20);
  });

}  //end callbackset_14_4_AO1_OUTPUTCURRENT_20ma_succweb


void callbackset_2_2_2_1_1_AO1_OUTPUTVOLT0Vsuccweb(void) 
{
  
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTVOLT0V_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 0v   set succed  </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out is 0v </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
     usr_io_instance.ao_volt_set(1, 1, 0);
  });

}  //end callbackset_15_1_1_AO1_OUTPUTVOLT0Vsuccweb

void callbackset_2_2_2_1_2_AO1_OUTPUTVOLT3Vsuccweb(void) {
  //unsigned char out2status=0;
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTVOLT3V_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 3v  set succed  </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out is 3v </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");

    usr_io_instance.ao_volt_set(1, 1, 3);
  });

}  //end callbackset_13_2_AO1_OUTPUTVOLT3Vsuccweb)

void callbackset_2_2_2_1_3_AO1_OUTPUTVOLT6Vsuccweb(void) {
  //unsigned char out2status=0;
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTVOLT6V_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 6v  set succed  </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out is 6v </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
    usr_io_instance.ao_volt_set(1, 1, 6);
  });

}  

void callbackset_2_2_2_1_4_AO1_OUTPUTVOLT10Vsuccweb(void) {
  
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTVOLT10V_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>ao-1 out is 10v  set succed  </title>\
      </head>\
      <body>\
        <h1>:  ao-1 out is 10v </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");

    usr_io_instance.ao_volt_set(1, 1, 10);
  });

}  //end callbackset_15_1_4_AO1_OUTPUTVOLT10Vsuccweb

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//unsigned char out2status = 0;



void callbackset_1_2_3_DOUT2_ONsuccweb(void) {
  //unsigned char out1status=0;
  AsyncWeb_server_instance.on("/PAGE_OUT2_ONBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>DOUT  set succed  </title>\
      </head>\
      <body>\
        <h1>:  press return BUTTON to reback root web </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_doutconfig_ONBUTTON\"'>\
        </form>\
      </body>\
    </html>");
    eg118_drvout.M100DOU2_ON();
  });

}  //end callbackset_1_3_9_DOUT2_ONsuccweb

void callbackset_1_2_4_DOUT2_OFFsuccweb(void) {
  //unsigned char out1status=0;

  AsyncWeb_server_instance.on("/PAGE_OUT2_OFFBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>DOUT  set succed  </title>\
      </head>\
      <body>\
        <h1>:  press return BUTTON to reback root web </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_doutconfig_ONBUTTON\"'>\
        </form>\
      </body>\
    </html>");
    eg118_drvout.M100DOU2_OFF();
  });

}  //end callbackset_1_4_9_DOUT2_OFFsuccweb


void callbackset_1_2_5_get_AI_succweb(void) {
  AsyncWeb_server_instance.on("/PAGE_GET_EG118_AI_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", EG118_ai_1_info);
  });
}  //END callbackset_1_5_9_get_AI_succweb


void callbackset_1_2_6_get_eg118DI_succweb(void) 
{

 pinMode(39, INPUT);
 snprintf(EG118_di_1_info, 20, " eg118 DI1 is %d . \n", digitalRead(39));
  AsyncWeb_server_instance.on("/PAGE_get_EG118_DI_BUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", EG118_di_1_info);
  });
}  //END callbackset_1_5_9_get_AI_succweb



void callbackset_1_4_2connect_to_net_via_ethernet_web(void) {
  AsyncWeb_server_instance.on("/PAGE_CONNECT_MODE_ETHERNET_INFO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_SET_AT_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>:  It is necessary to ensure that the router connected to the network cable can connect to the Internet normally. </h1>\
        <form>\
             <input type='button' value='ok' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_CONNECT_TO_NET_WITH_ETHERNETMODE();
    eg118_drvout.M100DOU2_OFF();
    delay(500);
    eg118_drvout.M100DOU2_ON();
  });
}


void callbackset_1_4_1connect_to_net_via_wifi_web(void) {
  AsyncWeb_server_instance.on("/PAGE_CONNECT_MODE_WIFI_INFO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_CONNECT_MODE_WIFI  set succed  </title>\
      </head>\
      <body>\
        <h1>:  It is necessary to ensure that WIFI can connect to the Internet normally. \
         </h1>\
        <form>\
             <input type='button' value='ok' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_CONNECT_TO_NET_WITH_WIFIMODE();
  });
}

void callbackset_1_5_usrio_configweb(void) {
  AsyncWeb_server_instance.on("/PAGE_USRIOCONFIG_ONBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "\
    <html><body>\
    <form action=\"/process\">\
    <div class=\"row form-group\">\
        <label  class=\"col-md-3 side-right\" for=\"slot1_usriotype\">type of device in slot1  :</label>\
    </div>\
    <div class=\"col-md-6\">\
        <select name=\"slot1_usriotype\" id=\"slot1_usriotype\">\
      <option value=\"USR_IO4040\"> USR_IO4040 </option>\
      <option value=\"USR_IO0440\"> USR_IO0440</option>\
      <option value=\"USR_IO0080\">USR_IO0080</option>\
      <option value=\"USR_IO8000\">USR_IO8000</option>\
      <option value=\"USR_IO0800\">USR_IO0800</option>\
      <option value=\"USR_IO0404\">USR_IO0404</option>\
     </select>\
     </div>\
    <div class=\"row form-group\">\
    <label  class=\"col-md-3 side-right\" for=\"slot2_usriotype\">type of device in slot2 :</label>\ 
    </div>\
    <div class=\"col-md-6\">\
            <select name=\"slot2_usriotype\" id=\"slot2_usriotype\">\
      <option value=\"USR_IO4040\"> USR_IO4040 </option>\
      <option value=\"USR_IO0440\"> USR_IO0440</option>\
      <option value=\"USR_IO0080\">USR_IO0080</option>\
      <option value=\"USR_IO8000\">USR_IO8000</option>\
      <option value=\"USR_IO0800\">USR_IO0800</option>\
      <option value=\"USR_IO0404\">USR_IO0404</option>\
     </select>\
    </div>\
    <div class=\"row form-group\">\
    <label  class=\"col-md-3 side-right\" for=\"slot3_usriotype\">type of device in slot3 :</label>\ 
    </div>\
    <div class=\"col-md-6\">\
                <select name=\"slot3_usriotype\" id=\"slot3_usriotype\">\
      <option value=\"USR_IO4040\"> USR_IO4040 </option>\
      <option value=\"USR_IO0440\"> USR_IO0440</option>\
      <option value=\"USR_IO0080\">USR_IO0080</option>\
      <option value=\"USR_IO8000\">USR_IO8000</option>\
      <option value=\"USR_IO0800\">USR_IO0800</option>\
      <option value=\"USR_IO0404\">USR_IO0404</option>\
     </select>\
    </div>\
    <input type='submit'value='submit'>\
    <input type='reset'value='reset'>\
    </form>\
    </body>\
    </html>");
  });
  AsyncWeb_server_instance.on("/process", HTTP_GET, [](AsyncWebServerRequest *request) {

      if (request->hasParam("slot1_usriotype")) 
      {
        String slot1_usriotypmessage;
        slot1_usriotypmessage = request->getParam("slot1_usriotype")->value();
        slot1_usriotypmessage.toCharArray(g_text_slot1_usriotype_stringbuf, 50, 0);
        adjustarray(g_text_slot1_usriotype_stringbuf, 50);
        
        eg118_slot1_usriotype = usrioname_todevice_id(g_text_slot1_usriotype_stringbuf);
        system_parameter_eg118_main_moudle.set_slot1_usrio_type(eg118_slot1_usriotype);
    }

      if (request->hasParam("slot2_usriotype")) 
      {
        String slot2_usriotypmessage;
        slot2_usriotypmessage = request->getParam("slot2_usriotype")->value();
        memset(g_text_slot2_usriotype_stringbuf,0,50);
        slot2_usriotypmessage.toCharArray(g_text_slot2_usriotype_stringbuf, 50, 0);
        adjustarray(g_text_slot2_usriotype_stringbuf, 50);
        
        eg118_slot2_usriotype = usrioname_todevice_id(g_text_slot2_usriotype_stringbuf);
        system_parameter_eg118_main_moudle.set_slot2_usrio_type(eg118_slot2_usriotype);
    }
    if (request->hasParam("slot3_usriotype")) 
    {
      String slot3_usriotypmessage;
      slot3_usriotypmessage = request->getParam("slot3_usriotype")->value();
      memset(g_text_slot2_usriotype_stringbuf,0,50);
      slot3_usriotypmessage.toCharArray(g_text_slot3_usriotype_stringbuf, 50, 0);
      adjustarray(g_text_slot3_usriotype_stringbuf, 50);
      eg118_slot3_usriotype = usrioname_todevice_id(g_text_slot3_usriotype_stringbuf);
      system_parameter_eg118_main_moudle.set_slot3_usrio_type(eg118_slot3_usriotype);
    }

#ifdef PCMODBUSTOOLDEMO_ENABLE
    if (request->hasParam("holdregisteraddress")) 
    {
      int v_holdregisteraddress;
      String holdregisteraddress_message;
      holdregisteraddress_message = request->getParam("holdregisteraddress")->value();
      holdregisteraddress_message.toCharArray(text_HOLD_addressstringbuf, 10, 0);
      v_holdregisteraddress = atoi(text_HOLD_addressstringbuf);
       modbus_demo_instance.setholdregisteraddress(v_holdregisteraddress);
    }
    if (request->hasParam("holdregister_writevalue")) 
    {
      int v_holdregistervalue;
      String holdregister_writevaluemessage;
      holdregister_writevaluemessage = request->getParam("holdregister_writevalue")->value();
      holdregister_writevaluemessage.toCharArray(text_HOLD_valuestringbuf, 10, 0);
      v_holdregistervalue = atoi(text_HOLD_valuestringbuf);
      modbus_demo_instance.setholdregistervalue(v_holdregistervalue);
    }
#endif
  });
}





void usrioconfig_ao_output_volt_mode(void)
{
    //usr_io_instance.init();
    usr_io_instance.ao_config(1, 1, 1);
}


void callbackset_2_2_1_1_DO1_onweb(void) 
{
  
  AsyncWeb_server_instance.on("/PAGE_USRIO_DO1_ON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_USRIO_DO1_ON   set succed  </title>\
      </head>\
      <body>\
        <h1>:  PAGE_USRIO_DO1_ON </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_DO_DEMO\"'>\
        </form>\
      </body>\
    </html>");
     usr_io_instance.do_set(2, 1, 1);
  });

}  //end callbackset_14_1_DO1_onweb

void callbackset_2_2_1_2_DO1_offweb(void) 
{
  
  AsyncWeb_server_instance.on("/PAGE_USRIO_DO1_OFF", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_USRIO_DO1_OFF   set succed  </title>\
      </head>\
      <body>\
        <h1>:  PAGE_USRIO_DO1_OFF </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_DO_DEMO\"'>\
        </form>\
      </body>\
    </html>");
     usr_io_instance.do_set(2, 1, 0);
  });

}  //end callbackset_14_2_DO1_offweb
void callbackset_2_2_1_3_DO2_onweb(void) 
{
  AsyncWeb_server_instance.on("/PAGE_USRIO_DO2_ON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_USRIO_DO2_ON   set succed  </title>\
      </head>\
      <body>\
        <h1>:  PAGE_USRIO_DO2_ON </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_DO_DEMO\"'>\
        </form>\
      </body>\
    </html>");
     usr_io_instance.do_set(2, 2, 1);
  });

}  //end callbackset_14_3_DO2_onweb
void callbackset_2_2_1_4_DO2_offweb(void) 
{
  
  AsyncWeb_server_instance.on("/PAGE_USRIO_DO2_OFF", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_USRIO_DO2_OFF   set succed  </title>\
      </head>\
      <body>\
        <h1>:  PAGE_USRIO_DO2_OFF </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_DO_DEMO\"'>\
        </form>\
      </body>\
    </html>");
     usr_io_instance.do_set(2, 2, 0);
  });

}  //end callbackset_14_2_DO1_OFFweb
void callbackset_2_2_2_demo_usrio_AO_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_USRIO_AO_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  USR-EG118 set USER_IO_AO-1_DEMO .  usrio device is in slot 1.  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118  user_io  AO demo usrio device is in slot 1. ] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
            <input type='button' value='AO-1_outvolt_value' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT_VALUE_LIST\"'>\
            <input type='button' value='AO-1_outcurrent_value' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
    
  });
}
void callbackset_2_2_3_demo_usrio_ai_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_USRIO_AI_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  USR-EG118 GET USER_IO_AI DEMO .  usrio device is in slot 1.  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 GET USER_IO_AI (ai1 ai2 ai3 ai4) DEMO usrio device is in slot 1. ] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_EG118_DEMO\"'>\
            <input type='button' value='get_usrioAIvalue' onclick='document.location=\"/PAGE_GET_AI_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
  });
}

void callbackset_2_2_4_demo_usrio_di_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_USRIO_DI_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  USR-EG118 GET USER_IO_DI DEMO .  usrio device is in slot 2.  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 GET USER_IO_DI  DEMO  device is in slot 2. Voltage greater than 12v   di status is 1] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_DEMO\"'>\
            <input type='button' value='get_usrioDIvalue' onclick='document.location=\"/PAGE_GET_DI_VALUE_LIST\"'>\
        </form>\
      </body>\
    </html>");
  });
}
void callbackset_2_2_3_1_getusrio_aivaluelist_succweb(void) 
{
  AsyncWeb_server_instance.on("/PAGE_GET_AI_VALUE_LIST", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", usrio_ai_info);
  });
}  // 


void callbackset_2_2_4_1_getusrio_divaluelist_succweb(void) 
{
  AsyncWeb_server_instance.on("/PAGE_GET_DI_VALUE_LIST", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", usrio_di_info);
  });
}  // 




void callbackset_1_eg118_config_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_EG118_CONFIG", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  EG118_CONFIG .   </title>\
      </head>\
      <body>\
        <h1>:  -[EG118_CONFIG ] </h1>\
        <form>\
            <input type='button' value='1 wifi_sta' onclick='document.location=\"/PAGE_wifi_sta_modeBUTTON\"'>\
            <input type='button' value='2 eg118_io_config' onclick='document.location=\"/PAGE_doutconfig_ONBUTTON\"'>\
            <input type='button' value='3 wifi_softap' onclick='document.location=\"/PAGE_wifi_SOFTAP_modeBUTTON\"'>\
             <input type='button' value='connect_mode' onclick='document.location=\"/PAGE_CONNECT_MODE\"'>\
             <input type='button' value='5 usrioconfig' onclick='document.location=\"/PAGE_USRIOCONFIG_ONBUTTON\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    
  });
}
void callbackset_1_1_STA_MODEweb(void) {
  AsyncWeb_server_instance.on("/PAGE_wifi_sta_modeBUTTON", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>wifi mode set sta  </title>\
      </head>\
      <body>\
        <h1>:      SET USR-EG118 to STA_MODE MODE when USR-EG118 POWER ON </h1>\
        <form>\
          <input type='button' value='sta_wifisetting' ' onclick='document.location=\"/wifi_sta_settingPAGE\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_EG118_CONFIG\"'>\
        </form>\
      </body>\
    </html>");

    system_parameter_eg118_main_moudle.set_wifi_status(EEPROM_WIFI_STA_MODE);
  });
}  //end callbackset_1_1_STA_MODEweb
void callbackset_1_1_1wifistasettingweb(void) {
  AsyncWeb_server_instance.on("/wifi_sta_settingPAGE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "\
    <html><body>\
    <form action=\"/process\">\
    <div class=\"row form-group\">\
        <label  class=\"col-xs-3 side-right\" for=\"wifi_ssid\">ssid (maxlen 8) :</label>\
    </div>\
    <div class=\"col-xs-6\">\
    <input type='text' id='wifi_ssid' name='wifi_ssid'>\
     </div>\
    <div class=\"row form-group\">\
    <label  class=\"col-xs-3 side-right\" for=\"wifi_passward\">password (maxlen 8) :</label>\ 
    </div>\
    <div class=\"col-xs-6\">\
    <input type='text' id='wifi_passward' name='wifi_passward'>\
    </div>\
    <input type='submit'value='submit'>\
    <input type='reset'value='reset'>\
    </form>\
    </body>\
    </html>");
  });
  AsyncWeb_server_instance.on("/process", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("wifi_ssid")) 
    {
      String ssidmessage;
      ssidmessage = request->getParam("wifi_ssid")->value();
      ssidmessage.toCharArray(text_SSIDstringbuf, WIFI_SSIDstring_LENGTH_MAX, 0);
      save_sta_wifi_info_toeeprom(text_SSIDstringbuf,text_PASSWARD_stringbuf);
    }

    if (request->hasParam("wifi_passward")) 
    {
      String wifi_passwardmessage;
      wifi_passwardmessage = request->getParam("wifi_passward")->value();
      memset(text_PASSWARD_stringbuf, 0, WIFI_PASSWARDstring_LENGTH_MAX);
      wifi_passwardmessage.toCharArray(text_PASSWARD_stringbuf, WIFI_PASSWARDstring_LENGTH_MAX, 0);
      
      save_sta_wifi_info_toeeprom(text_SSIDstringbuf,text_PASSWARD_stringbuf);

    }  // if (request->hasParam("wifi_passward"))

    if (request->hasParam("tcpserver_ip")) 
    {

      String tcpserver_ipmessage;

      tcpserver_ipmessage = request->getParam("tcpserver_ip")->value();
      tcpserver_ipmessage.toCharArray(text_tcpserverip_stringbuf, TCPSERVER_IPstring_length_MAX, 0);
     savetcpserverinfo_toeeprom(text_tcpserverip_stringbuf,text_tcpserverport_stringbuf);
    }

    if (request->hasParam("modbustcpserver_ip")) {

      String modbustcpserver_ipmessage;

      modbustcpserver_ipmessage = request->getParam("modbustcpserver_ip")->value();
      modbustcpserver_ipmessage.toCharArray(text_modbustcpserverip_stringbuf, MODBUSTCPSERVERstring_length_MAX, 0);
     save_modbustcpserverinfo_toeeprom(text_modbustcpserverip_stringbuf);
    }

    if (request->hasParam("tcpserver_port")) {
      String tcpserver_portmessage;
      tcpserver_portmessage = request->getParam("tcpserver_port")->value();
      tcpserver_portmessage.toCharArray(text_tcpserverport_stringbuf, TCPSERVER_PORTstring_length_MAX, 0);
      savetcpserverinfo_toeeprom(text_tcpserverip_stringbuf,text_tcpserverport_stringbuf);
    }  // if (request->hasParam("tcpserver_port"))
    if (request->hasParam("mqttserver_ip")) {
      String mqttserverip_message;

      mqttserverip_message = request->getParam("mqttserver_ip")->value();
      mqttserverip_message.toCharArray(text_mqttserverip_stringbuf, 50, 0);
      savemqttserverinfo_toeeprom(text_mqttserverip_stringbuf,text_mqttserveruser_stringbuf,text_mqttserverpass_stringbuf);
    }

    if (request->hasParam("mqttserver_user")) {
      String mqttserver_usermessage;
      mqttserver_usermessage = request->getParam("mqttserver_user")->value();
      mqttserver_usermessage.toCharArray(text_mqttserveruser_stringbuf, 10, 0);
      savemqttserverinfo_toeeprom(text_mqttserverip_stringbuf,text_mqttserveruser_stringbuf,text_mqttserverpass_stringbuf);
    }
    if (request->hasParam("mqttserver_pass")) {
      String mqttserver_passmessage;
      mqttserver_passmessage = request->getParam("mqttserver_pass")->value();
      mqttserver_passmessage.toCharArray(text_mqttserverpass_stringbuf, MQTTSERVER_PASS_length_MAX, 0);
      savemqttserverinfo_toeeprom(text_mqttserverip_stringbuf,text_mqttserveruser_stringbuf,text_mqttserverpass_stringbuf);
    }

      if (request->hasParam("slot1_usriotype")) 
      {
        String usrio_slot1_typmessage;
        usrio_slot1_typmessage = request->getParam("slot1_usriotype")->value();
        usrio_slot1_typmessage.toCharArray(g_text_slot1_usriotype_stringbuf, 50, 0);
        adjustarray(g_text_slot1_usriotype_stringbuf, 50);
        
        eg118_slot1_usriotype = usrioname_todevice_id(g_text_slot1_usriotype_stringbuf);
        system_parameter_eg118_main_moudle.set_slot1_usrio_type(eg118_slot1_usriotype);
    }

      if (request->hasParam("slot2_usriotype")) 
      {
        String usrio_slot2_typmessage;
        usrio_slot2_typmessage = request->getParam("slot2_usriotype")->value();
        usrio_slot2_typmessage.toCharArray(g_text_slot2_usriotype_stringbuf, 50, 0);
        adjustarray(g_text_slot2_usriotype_stringbuf, 50);
        
        eg118_slot2_usriotype = usrioname_todevice_id(g_text_slot2_usriotype_stringbuf);
        system_parameter_eg118_main_moudle.set_slot2_usrio_type(eg118_slot2_usriotype);
    }
    if (request->hasParam("slot3_usriotype")) 
    {
      String usrioslot3_typmessage;
      usrioslot3_typmessage = request->getParam("slot3_usriotype")->value();
      usrioslot3_typmessage.toCharArray(g_text_slot3_usriotype_stringbuf, 50, 0);
      adjustarray(g_text_slot3_usriotype_stringbuf, 50);
      eg118_slot3_usriotype = usrioname_todevice_id(g_text_slot3_usriotype_stringbuf);
      system_parameter_eg118_main_moudle.set_slot3_usrio_type(eg118_slot3_usriotype);
    }

#ifdef PCMODBUSTOOLDEMO_ENABLE
    if (request->hasParam("holdregisteraddress")) 
    {
      int v_holdregisteraddress;
      String holdregisteraddress_message;
      holdregisteraddress_message = request->getParam("holdregisteraddress")->value();
      holdregisteraddress_message.toCharArray(text_HOLD_addressstringbuf, 10, 0);
      v_holdregisteraddress = atoi(text_HOLD_addressstringbuf);
      modbus_demo_instance.setholdregisteraddress(v_holdregisteraddress);

    }
    if (request->hasParam("holdregister_writevalue")) 
    {
      int v_holdregistervalue;
      String holdregister_writevaluemessage;
      holdregister_writevaluemessage = request->getParam("holdregister_writevalue")->value();
      holdregister_writevaluemessage.toCharArray(text_HOLD_valuestringbuf, 10, 0);
      v_holdregistervalue = atoi(text_HOLD_valuestringbuf);
      modbus_demo_instance.setholdregistervalue(v_holdregistervalue);
    }
#endif
  });
}


void callbackset_2_eg118_demo_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_EG118_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  USR-EG118 demo .   </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 demo ] </h1>\
        <form>\  
            <input type='button' value='1 iot cloud demo' onclick='document.location=\"/PAGE_IOTCLOUD_DEMO\"'>\
            <input type='button' value='2 usriodemo' onclick='document.location=\"/PAGE_USRIO_DEMO\"'>\
            <input type='button' value='3 modbusCONFIG' onclick='document.location=\"/PAGE_MODBUSCONFIG_MODE\"'>\
            <input type='button' value='4 modifyslave_device' onclick='document.location=\"/PAGE_RS485_MODBUS_SLAVESETTING\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    
  });
}


void callbackset_2_2_usrio_demo_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_USRIO_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  PAGE_USRIO_DEMO .   </title>\
      </head>\
      <body>\
        <h1>:  -[PAGE_USRIO_DEMO ] </h1>\
        <form>\
            <input type='button' value='1 usrio do demo' onclick='document.location=\"/PAGE_USRIO_DO_DEMO\"'>\
            <input type='button' value='2 usrio ao demo' onclick='document.location=\"/PAGE_USRIO_AO_DEMO\"'>\
            <input type='button' value='3 usrio ai demo' onclick='document.location=\"/PAGE_USRIO_AI_DEMO\"'>\
            <input type='button' value='4 usrio di demo' onclick='document.location=\"/PAGE_USRIO_DI_DEMO\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    
  });
}//callbackset_2_2_usrio_demo_web


void callbackset_2_3_modbus_config_mode_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_MODBUSCONFIG_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  PAGE_MODBUS_DEMO .   </title>\
      </head>\
      <body>\
        <h1>:  -[PAGE_MODBUS_DEMO ] </h1>\
        <form>\
            <input type='button' value=' rtu demo' onclick='document.location=\"/PAGE_MODBUS_RTU_DEMO\"'>\
            <input type='button' value=' tcp demo' onclick='document.location=\"/PAGE_MODBUS_TCP_DEMO\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_EG118_DEMO\"'>\
        </form>\
      </body>\
    </html>");
    
  });
}//callbackset_2_3_modbus_demo_web


void callbackset_2_1iotcloud_demo_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_IOTCLOUD_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  PAGE_IOTCLOUD_DEMO .   </title>\
      </head>\
      <body>\
        <h1>:  -[PAGE_IOTCLOUD_DEMO ] </h1>\
        <form>\
            <input type='button' value='1 huaweicloud demo' onclick='document.location=\"/PAGE_HUAWEI_CLOUD_IOT_MODE\"'>\
            <input type='button' value='2 aws_iot_demo' onclick='document.location=\"/PAGE_AWS_CLOUD_IOT_MODE\"'>\
             <input type='button' value='return' ' onclick='document.location=\"/PAGE_EG118_DEMO\"'>\
        </form>\
      </body>\
    </html>");
    
  });
}//callbackset_2_1_eg118_demo_web


void callbackset_2_1_1_huaweiiot_cloud_web(void) {
  AsyncWeb_server_instance.on("/PAGE_HUAWEI_CLOUD_IOT_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>huawei_cloud_iot_only_SAMPLE_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>USR-EG118: [SET USR-EG118 connect to huawei_iot  by 05.23.  -->                                                ] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_IOTCLOUD_DEMO\"'>\
        </form>\
      </body>\
    </html>");
    //EEPROM_setdeviceMode_HUAWEI_IOT_NOSSL();
    system_parameter_eg118_main_moudle.set_deviceMode_HUAWEI_IOT_NOSSL();
  });
}


void callbackset_2_1_2_awsiot_cloud_web(void) {
  AsyncWeb_server_instance.on("/PAGE_AWS_CLOUD_IOT_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_cloud_iot_only_SAMPLE_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>USR-EG118: [SET USR-EG118 connect to AWS_iot  by 09.29.  -->                                                ] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_HUAWEI_CLOUD_IOT_MODE\"'>\
        </form>\
      </body>\
    </html>");
    //EEPROM_setdeviceMode_AWS_IOT();
    system_parameter_eg118_main_moudle.set_deviceMode_AWS_IOT();
  });
}




void callbackset_2_2_1_demo_usrio_do_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_USRIO_DO_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  USR-EG118 set USER_IO DO ON OFF .   notice:  usrio device is in slot 2.succed  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 set user_io DO CONTROL ] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_DEMO\"'>\
            <input type='button' value='1 USRIO_DO1_ON' ' onclick='document.location=\"/PAGE_USRIO_DO1_ON\"'>\
            <input type='button' value='2 USRIO_DO1_OFF' ' onclick='document.location=\"/PAGE_USRIO_DO1_OFF\"'>\
            <input type='button' value='3 USRIO_DO2_ON' ' onclick='document.location=\"/PAGE_USRIO_DO2_ON\"'>\
            <input type='button' value='4 USRIO_DO2_OFF' onclick='document.location=\"/PAGE_USRIO_DO2_OFF\"'>\
        </form>\
      </body>\
    </html>");
  });
}

void callbackset_2_3_1_setmodbusmode_rtu_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_MODBUS_RTU_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  PAGE_MODBUS_RTU_DEMO .   PC modbus start tool Modbus Slave  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 set EG118 MODBUS RTU.when eg118 repower on.  id is 1 BAUDis 115200 . holdregister addr start from  0  ] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_deviceMode_MODBUS_RTU();
  });
}
void callbackset_2_3_2_setmodbusmode_tcp_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_MODBUS_TCP_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  PAGE_MODBUS_TCP_DEMO .   PC modbus start tool Modbus Slave  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 set device id is 1 modbus tcpserver ip setting . holdregister addr start from  0  ] </h1>\
        <form>\
         <input type='button' value='modbustcpserver' ' onclick='document.location=\"/PAGE_RS485_MODBUS_TCP_SERVERSETTING\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_deviceMode_MODBUS_TCP();
  });
}
void callbackset_3_eg118_rs485work_func_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_RS485_WORKFUNC", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  USR-EG118 demo .   </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 RS485_WORKFUNC ] </h1>\
        <form>\
            <input type='button' value='1 RS485AT' onclick='document.location=\"/PAGE_RS485AT_MODE\"'>\
            <input type='button' value='2 RS485_MQTT_DTU' onclick='document.location=\"/PAGE_RS485_MQTTDTU_MODE\"'>\
            <input type='button' value='3 RS485_TCPDTU' onclick='document.location=\"/PAGE_RS485TCPDTU_MODE\"'>\
             <input type='button' value='4 th485pe demo' onclick='document.location=\"/PAGE_TH485PE_DEMO\"'>\
            <input type='button' value=' return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    
  });
}

void callbackset_3_4_demo_TH485PE_web(void) {
  AsyncWeb_server_instance.on("/PAGE_TH485PE_DEMO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>TH485PE_DEMO  set succed  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 read humidity and tempreture from LH-TH485PE  when USR-EG118 POWER ON][SLAVEDEVICEID is 1] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_deviceMode_TH485PE_DEMO();
  });
}
//currentsystem_info
void callbackset_4_get_SYSTEM_INFO_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_SYSTEM_INFO", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", system_info);
  });
}  // 



void callbackset_3_1_rs485work_func_at_web(void) {
  AsyncWeb_server_instance.on("/PAGE_RS485AT_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>PAGE_SET_AT_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>:  SET USR-EG118 rs485 atcommand mode when USR-EG118 POWER ON </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_RS485_WORKFUNC\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_deviceMode_AT_MODE();
  });
}


void callbackset_3_2_rs485work_func_mqttdtu_web(void) {
  AsyncWeb_server_instance.on("/PAGE_RS485_MQTTDTU_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>MQTT_DTU_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>USR-EG118:  SET USR-EG118 work rs485 date transparent transmission MODE over mqtt connect when USR-EG118 POWER ON </h1>\
        <form>\
            <input type='button' value='mqttserversetting' ' onclick='document.location=\"/mqttserversettingPAGE\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_deviceMode_MQTT_DTU_ONLY();
  });
}  //callbackset_3_2_rs485work_func_mqttdtu_web

void callbackset_3_3_rs485work_func_tcpdtu_web(void) {
  AsyncWeb_server_instance.on("/PAGE_RS485TCPDTU_MODE", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>tcp_DTU_MODE  set succed  </title>\
      </head>\
      <body>\
        <h1>USR-EG118:  SET USR-EG118 work serial date ontransparent transmission MODE over tcp connect when USR-EG118 POWER ON </h1>\
        <form>\
            <input type='button' value='tcpserversetting' ' onclick='document.location=\"/tcpserversettingPAGE\"'>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_reback_to_root_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    system_parameter_eg118_main_moudle.set_deviceMode_TCP_DTU_ONLY();
  });
}//callbackset_3_3_rs485_tcpdtu_workfunc_web


void callbackset_2_2_2_1_AO1_OUTPUTVOLT_VALUElist_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTVOLT_VALUE_LIST", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>  USR-EG118 set USER_IO_AO-1_OUT Analog volt list .   notice:  usrio device is in slot 1.succed  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 set user_io AO-1_OUT_VOLT LIST] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_AO_DEMO\"'>\
            <input type='button' value=' 0V' ' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT0V_BUTTON\"'>\
            <input type='button' value=' 3V' ' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT3V_BUTTON\"'>\
            <input type='button' value=' 6V' ' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT6V_BUTTON\"'>\
            <input type='button' value=' 10V' onclick='document.location=\"/PAGE_AO1_OUTPUTVOLT10V_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    usrioconfig_ao_output_volt_mode();
  });
}

void usrioconfig_ao_output_current_mode(void)
{
   // usr_io_instance.init();
    usr_io_instance.ao_config(1, 1, 0);
}
void callbackset_2_2_2_2_AO1_OUT_CURRENT_VALUElist_web(void) 
{
  AsyncWeb_server_instance.on("/PAGE_AO1_OUTPUTCURRENT_VALUE_LIST", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html",
                    "<html>\
      <head>\
        <title>USER_IO AO_OUT_CURRENT_DEMO  set succed  </title>\
      </head>\
      <body>\
        <h1>:  -[USR-EG118 set USER_IO_AO-1_OUT as Analog CURRENT value list.    notice:usrio device is in slot 1.] </h1>\
        <form>\
            <input type='button' value='return' ' onclick='document.location=\"/PAGE_USRIO_AO_DEMO\"'>\
                    <input type='button' value=' 4ma' ' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_4ma_BUTTON\"'>\
                    <input type='button' value=' 5ma' ' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_5ma_BUTTON\"'>\
                    <input type='button' value=' 10ma' ' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_10ma_BUTTON\"'>\
                    <input type='button' value=' 20ma' onclick='document.location=\"/PAGE_AO1_OUTPUTCURRENT_20ma_BUTTON\"'>\
        </form>\
      </body>\
    </html>");
    usrioconfig_ao_output_current_mode();

  });
}



void configcallback(void) {

  callbackset_0_m100rootweb();
  callbakset_0_m100_reback_to_root_web();
  callbackset_1_eg118_config_web();
    callbackset_1_1_STA_MODEweb();
      callbackset_1_1_1wifistasettingweb();
    callbackset_1_2_doutconfig_web();
      callbackset_1_2_1_DOUT1_ONsuccweb();
      callbackset_1_2_2_DOUT1_OFFsuccweb();
      callbackset_1_2_3_DOUT2_ONsuccweb();
      callbackset_1_2_4_DOUT2_OFFsuccweb();
      callbackset_1_2_5_get_AI_succweb();
      callbackset_1_2_6_get_eg118DI_succweb();
    callbackset_1_3_softapmode_web();
    callbackset_1_4_connecttonetmode_web();
        callbackset_1_4_1connect_to_net_via_wifi_web();
        callbackset_1_4_2connect_to_net_via_ethernet_web();
    callbackset_1_5_usrio_configweb();
 //////////////////////////////////////////////////////////////       
callbackset_2_eg118_demo_web();
    callbackset_2_1iotcloud_demo_web();
          callbackset_2_1_1_huaweiiot_cloud_web();
          callbackset_2_1_2_awsiot_cloud_web();
    callbackset_2_2_usrio_demo_web();
      callbackset_2_2_1_demo_usrio_do_web();
        callbackset_2_2_1_1_DO1_onweb();
        callbackset_2_2_1_2_DO1_offweb();
        callbackset_2_2_1_3_DO2_onweb();
        callbackset_2_2_1_4_DO2_offweb();

      callbackset_2_2_2_demo_usrio_AO_web();
        callbackset_2_2_2_1_AO1_OUTPUTVOLT_VALUElist_web();
          callbackset_2_2_2_1_1_AO1_OUTPUTVOLT0Vsuccweb();
          callbackset_2_2_2_1_2_AO1_OUTPUTVOLT3Vsuccweb();
          callbackset_2_2_2_1_3_AO1_OUTPUTVOLT6Vsuccweb();
          callbackset_2_2_2_1_4_AO1_OUTPUTVOLT10Vsuccweb();
        callbackset_2_2_2_2_AO1_OUT_CURRENT_VALUElist_web();
          callbackset_2_2_2_2_1AO1_OUTPUTCURRENT_4ma_succweb();
          callbackset_2_2_2_2_2AO1_OUTPUTCURRENT_5ma_succweb();
          callbackset_2_2_2_2_3AO1_OUTPUTCURRENT_10ma_succweb();
          callbackset_2_2_2_2_4AO1_OUTPUTCURRENT_20ma_succweb();

      callbackset_2_2_3_demo_usrio_ai_web();
        callbackset_2_2_3_1_getusrio_aivaluelist_succweb();
      callbackset_2_2_4_demo_usrio_di_web();
        callbackset_2_2_4_1_getusrio_divaluelist_succweb();

      callbackset_2_3_modbus_config_mode_web();
        callbackset_2_3_1_setmodbusmode_rtu_web();
          callbackset2_3_2_1_modbustcpserversettingweb();
        callbackset_2_3_2_setmodbusmode_tcp_web();
  callbackset_2_4_modbus_slavedevice_setting_web(); 
callbackset_3_eg118_rs485work_func_web();
  callbackset_3_1_rs485work_func_at_web();
  callbackset_3_2_rs485work_func_mqttdtu_web();
    callbackset_3_2_1_mqttserversettingweb();
  callbackset_3_3_rs485work_func_tcpdtu_web();
    callbackset_3_3_1_tcpserversettingweb();
  callbackset_3_4_demo_TH485PE_web();
  callbackset_4_get_SYSTEM_INFO_web();

  }






static void wifi_connect_to_sta_router(void) {
  unsigned char ssid_length = 0;
  unsigned char password_length = 0;

  int i;


  memset(gv_ssidstring, 0, WIFI_SSIDstring_length_MAX);

  memset(gv_passward_string, 0, WIFI_PASSWARD_LENGTH_MAX);
  ssid_length = system_parameter_eg118_main_moudle.getwifiSSIDlength();

  system_parameter_eg118_main_moudle.getpoweronwifiSSID_string(gv_ssidstring, ssid_length);

  password_length = system_parameter_eg118_main_moudle.getwifi_password_length();

  system_parameter_eg118_main_moudle.getwifi_password__string(gv_passward_string, password_length);
  WiFi.mode(WIFI_STA);
  adjustarray(gv_ssidstring, WIFI_SSIDstring_length_MAX);
  adjustarray(gv_passward_string, WIFI_PASSWARD_LENGTH_MAX);

#ifdef LOG_ENABLE
  Serial.printf("[wifi_connect_to_sta_router]===LINE 1582->ssid_length=%d SSID=[%s]   \n", ssid_length, gv_ssidstring);
  Serial.printf("[wifi_connect_to_sta_router]===LINE 1584->password_length=%d PASSWORD=[%s]   \n", password_length, gv_passward_string);
#endif
  WiFi.begin(gv_ssidstring, gv_passward_string);
#ifdef LOG_ENABLE
  Serial.print("");
#endif

  // Wait for connection
  for (i = 0; i < 6; i++) {

    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    #ifdef LOG_ENABLE
    Serial.print(".");
    #endif
    reload_dog();

    delay(500);
  }
  #ifdef LOG_ENABLE
  Serial.printf("\n[wifi_connect_to_sta_router]===LINE 1601->i=%d max retrytime 6 times.\n", i);
  #endif
  //Serial.println("");
  if (i < 6) {
    #ifdef LOG_ENABLE
    Serial.printf("[wifi_connect_to_sta_router]===LINE 1604 ->wait %d times  succed Connected to ", i);

    Serial.println(gv_ssidstring);
    Serial.print("[wifi_connect_to_sta_router] ==LINE 1607->IP address: ");


    Serial.println(WiFi.localIP());
    Serial.print("[wifi_connect_to_sta_router] ==LINE 1609->IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[wifi_connect_to_sta_router] ==LINE 1611->IP address: ");
    Serial.println(WiFi.localIP());
    //gv_stawifi_rssi=WiFi.RSSI();
    #endif
  } else {
    #ifdef LOG_ENABLE
    Serial.print("[wifi_connect_to_sta_router] ==LINE 1614-> timeout wait 3 seconds .fail Connected to wifirouter \n");
    #endif
  }
}





void getcustommacstringbyreadsta_mac(char espread_macstring[]) {
  uint8_t wifi_sta_mac[6];
  memset(wifi_sta_mac, 0, 6);
  esp_read_mac(wifi_sta_mac, ESP_MAC_WIFI_STA);
  snprintf(espread_macstring, 5, "%02X%02X\n", wifi_sta_mac[4], wifi_sta_mac[5]);
}
void getcustommacstringbyreadlast4bytes_softap_mac(char espread_macstring[]) {
  uint8_t wifi_softap_mac[6];
  memset(wifi_softap_mac, 0, 6);
  esp_read_mac(wifi_softap_mac, ESP_MAC_WIFI_SOFTAP);
  snprintf(espread_macstring, 5, "%02X%02X\n", wifi_softap_mac[4], wifi_softap_mac[5]);
}
static void wifi_creat__softAP_router(void) 
{
  uint8_t wifi_ap_mac[6];
  unsigned char lastBYTE = 0;
  char macstring[BASE_MAC_LENGTH];
  char macstring_last4_BYTE[5];
  uint8_t base_maclength = 0;
  IPAddress LocalIP(192, 168, 1, 1);

  IPAddress SubNet(255, 255, 255, 0);
  uint8_t newMACAddress[6];
  //uint8_t setwifimac[6];
  //memset(newMACAddress, 0, 6);

  memset(macstring_last4_BYTE, 0, 5);

  //getcustommacstringbyreadsta_mac(custombase_macstring_4_BYTE);
  getcustommacstringbyreadlast4bytes_softap_mac(macstring_last4_BYTE);
  //getbase_macstring_byreadeeprom(macstring);
 // base_maclength = strlen(macstring);
  //strncpy(macstring_last4_BYTE, &macstring[base_maclength - 4], 4);
  //lastBYTE=atoi(macstring_last4_BYTE[3]);

  //lastBYTE=lastBYTE+1;
  //snprintf()
  //itoa(lastBYTE,&macstring_last4_BYTE[3],16);
  //Serial.printf("line  1699   custombase_macstring_4_BYTE->%s\n", custombase_macstring_4_BYTE);
  WiFi.mode(WIFI_AP);  // 设置为AP模式
  char ap_ssid[32] = { 0 };

  snprintf(ap_ssid, 32, "%s%s", softap_ssid, macstring_last4_BYTE);

 #ifdef LOG_ENABLE
  Serial.printf("[wifi_creat__softAP_router]====LINE 1665->ap_ssid->%s\n", ap_ssid);
#endif
  //Serial.println(ap_ssid);
  WiFi.softAPConfig(LocalIP, gv_apGateway, gv_apSubNet);
  WiFi.softAP(ap_ssid, softap_password);  // 创建WiFi接入点
  IPAddress ip = WiFi.softAPIP();         // 获取AP的IP地址
}
static void eg118start_wificonnect(void)
{
  if (eg118_wifi_mode == EEPROM_WIFI_STA_MODE) 
  {
    wifi_connect_to_sta_router();
  } 
  else 
  {
    wifi_creat__softAP_router();
    Serial.println("[setup]=========LINE 2651->wifi_creat__softAP_router");
  }
}
static void eg118start_ethconnect(void)
{
  #ifdef PHYTESTSAMPLE_ENABLE
  #ifdef LOG_ENABLE
      Serial.println("[eg118startethconnect]========================LINE 2388->ETH.begin");
      #endif

      ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLOCK_GPIO0_IN);  //
  //delay(1000);
  #endif

}


#ifdef PCMODBUSTOOLDEMO_ENABLE


#endif

#ifdef AWS_CLOUD_ENABLE


#endif


#ifdef TH485_PE_DEMO_ENABLE
void xTask_TH485_PE_DEMO_func(void *xTask1) {
  long humidity = 0;
  long temperature = 0;


  //Serial1.begin(LH_TH485PE_BAUD, SERIAL_8N1, RX1PIN, TX1PIN);
  reload_dog();
  if (!ModbusRTUClient.begin(RS485, LH_TH485PE_BAUD, SERIAL_8N1)) 
  {
     Serial.println("[setup]Failed to start Modbus RTU Client!");
  }
  reload_dog();
  while (1) 
  {
    humidity = ModbusRTUClient.holdingRegisterRead(LH_TH485D_DEVICE_ID, ADDRESS_LH_TH485D_HUMIDITY);
    Serial.printf("\n[xTask_TH485_PE_DEMO_func]=====LINE 1751->humidity= %d RH  \n", (humidity/10));
    delay(1000);
    temperature = ModbusRTUClient.holdingRegisterRead(LH_TH485D_DEVICE_ID, ADDRESS_LH_TH485D_TEMPTURE);
    Serial.printf("\n[xTask_USR-EG118_TH485_PE_DEMO_func]===LINE 1754->temperature= %d ℃  \n", (temperature/10));
    delay(2000);
  

  }
}  //xTask_TH485_PE_DEMO_func
#endif



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

void xTask_watchdog(void *xTask1) {

  while (1) {

    reload_dog();

    OneButton_buttonreload.tick();
    // delay(100);
    // OneButton_buttonreload.tick();
    if (reloadclickflag == 1) {
      reloadclickflag = 0;
      // irqtesthandle2();
      response_newcommandresult("reload", "CLICK\n");


      delay(110);
    }
    if (reloadclickflag == 2) {
      reloadclickflag = 0;
      // irqtesthandle2();
      response_newcommandresult(" reload:", "2X\n");
      delay(200);
    }
  }
}
void testcase_doset_instance(void)
{
  usr_io_instance.init();
  delay(3500);
  usr_io_instance.log_on(1);
   usr_io_instance.print();

  usr_io_instance.do_set(1, 4, 1);
 delay(1000);
usr_io_instance.do_set(1, 4, 0);
delay(1000);
usr_io_instance.do_set(1, 4, 1);
 delay(1000);
usr_io_instance.do_set(1, 4, 0);
delay(1000);
usr_io_instance.do_set(1, 4, 1);
 delay(1000);
usr_io_instance.do_set(1, 4, 0);
delay(1000);
usr_io_instance.do_set(1, 4, 1);
 delay(1000);
usr_io_instance.do_set(1, 4, 0);
delay(1000);

}




//在中断函数里面
//你不能使用delay( ) 函数。
//不仅如此，您也不能使用millis( ) 函数。
//没有串行库，因此无法打印到串行监视器。
//仅使用全局变量，这些变量应声明为volatile。

//static char led = 0;
//static char isinfallingisrprocess = 0;

void OneButton_buttonreloadlongpressup(void) {
  Serial.println("[OneButton_buttonreloadlongpressup]==line 156-> OneButton_buttonreloadlongpressup");
  irqtesthandle2();

  ereasecustom_info();
}  // doubleClick


//短按reload测试按键发送信息到485上。
void OneButton_buttonreloadClick(void) {
  //portENTER_CRITICAL_ISR(&pressclickMux);
  //irqtesthandle2();
  reloadclickflag = 1;
  //Serial.printf("LINE 2009 CLICK.CLICK gv_reloadclickflag=%d\n", gv_reloadclickflag);
  //response_newcommandresult(" [reload_pin]", "CLICK.CLICK\n");
  //portEXIT_CRITICAL_ISR(&pressclickMux);
}
void OneButton_buttonreloadpressstart(void) {
  //portENTER_CRITICAL_ISR(&pressclickMux);
  //irqtesthandle2();
  // gv_reloadclickflag=1;
  response_newcommandresult(" [reload_pin]", "pressstart\n");
  //portEXIT_CRITICAL_ISR(&pressclickMux);
}

void OneButton_buttonreloaddoubleClick(void) {

  // irqtesthandle2();

  reloadclickflag = 2;
  //response_newcommandresult(" [reload_pin]", "doubleClick\n");

}  // doubleClick

static void getbase_macstring_byreadeeprom(char get_base_macstring[]) {
  //int errorret = 0;

  //char custombase_macstring[BASE_MAC_LENGTH];

  uint8_t getbase_maclength = 0;

  char *endptr;
  long value;
  //uint8_t newMACAddress[6];
  //uint8_t setwifimac[6];
  //memset(newMACAddress, 0, 6);
  // memset(custombase_macstring, 0, BASE_MAC_LENGTH);
  //base_maclength = EEPROM_get_base_mac_length();
  getbase_maclength = system_parameter_eg118_main_moudle.get_basemac_length();

  //EEPROM_get_basemac_string(custombase_macstring, base_maclength);
  system_parameter_eg118_main_moudle.get_basemac_string(get_base_macstring, getbase_maclength);
  //修正字符串里面的换行符
  for (int j = 0; j < BASE_MAC_LENGTH; j++) {
    if (10 == get_base_macstring[j]) {
      get_base_macstring[j] = 0;
    }
  }
}

void setbasemac_stringto_esp32chip(char base_macstring[])
 {
  uint8_t hex_base_mac[6];
  int errorret = 0;
  char custombase_macstring[12];
  char mac_field[3];
  char *endptr;
  long value;
  uint8_t newMACAddress[6];
  //uint8_t setwifimac[6];
  memset(newMACAddress, 0, 6);
  memset(newMACAddress, 0, 6);
  memset(hex_base_mac, 0, 6);
  
  //system_parameter_eg118_main_moudle.get_basemac_string(custombase_macstring,12);
  //EEPROM_get_basemac_string(custombase_macstring);
  for (int i = 0; i < 6; i++) {
    mac_field[0] = base_macstring[2 * i];
    mac_field[1] = base_macstring[2 * i + 1];
    mac_field[2] = 0;
    //Serial.printf("[setbasemac_string]line  2017          tempmac  %s, \n", tempmac);
    value = strtol(mac_field, &endptr, 16);
    hex_base_mac[i] = value;
  }
  // newebase_mac[0] = newebase_mac[0] & 0xfe;
  errorret = esp_base_mac_addr_set(hex_base_mac);
  #ifdef LOG_ENABLE
  Serial.printf("[setbasemac_string]============LINE 1953->errorret= %x  base_macstring->%s\n",errorret, base_macstring);
  #endif
  //Serial.printf(" LINE 2048 base_mac 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", base_mac[0], base_mac[1], base_mac[2], base_mac[3], base_mac[4], base_mac[5]);
  //Serial.printf("[setbasemac_string]line  1999          esp_base_mac_addr_set  ret=0X%x, \n", errorret);
}

static void getbase_macstring_writetoesp32chip(void) 
{
  char custombase_macstring[BASE_MAC_LENGTH];
  uint8_t base_maclength = 0;

  memset(custombase_macstring, 0, BASE_MAC_LENGTH);
  //base_maclength = EEPROM_get_base_mac_length();
  base_maclength = system_parameter_eg118_main_moudle.get_basemac_length();

  //EEPROM_get_basemac_string(custombase_macstring, base_maclength);
  system_parameter_eg118_main_moudle.get_basemac_string(custombase_macstring, base_maclength);
  //修正字符串里面的换行符
  adjustarray(custombase_macstring,base_maclength);
  setbasemac_stringto_esp32chip(custombase_macstring);
}



void setup(void) 
{

  Serial.begin(9600);
  
  Serial1.begin(115200, SERIAL_8N1, RX1PIN, TX1PIN);
  RS485.begin(115200);
  //if (!ModbusRTUClient.begin(115200)) 

  reload_dog();

//////////////////////////////////////////////////////////////


  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
  Serial.println("USR-EG118 target power on,chip is esp32_wroom.build_time is ");
  //Serial.println("_ _DATE_ _");
  Serial.printf("                 compile date is   %s\n", __DATE__);
  Serial.printf("                 compile time is %s\n", __TIME__);

  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");



  system_parameter_eg118_main_moudle.parameter_config_start();
  system_parameter_eg118_main_moudle.parameter_refreshEEPROMRAM();
  //system_parameter_eg118_main_moudle.set_TOP_ADDR_VALUE(0xbb);


  
  eg118_connect_mode = system_parameter_eg118_main_moudle.GetconnectMode();
  eg118_device_mode = system_parameter_eg118_main_moudle.GetdeviceMode();
  eg118_wifi_mode = system_parameter_eg118_main_moudle.Getpoweron_wifi_status();

  eg118_slot1_usriotype=system_parameter_eg118_main_moudle.get_slot1_usrio_type();
  eg118_slot2_usriotype=system_parameter_eg118_main_moudle.get_slot2_usrio_type();
  eg118_slot3_usriotype=system_parameter_eg118_main_moudle.get_slot3_usrio_type();
#ifdef USRIO_DEMO_ENABLE
  usr_io_instance.slot_reg_device_id(eg118_slot1_usriotype,eg118_slot2_usriotype,eg118_slot3_usriotype);
  usr_io_instance.init();
#endif
#ifdef PHYTESTSAMPLE_ENABLE
  WiFi.onEvent(WiFiEvent);
#endif


//使用eth还是wifi，看调用顺序，后面的会覆盖前面的，所以后面的决定的了是采用哪种方式通信的。
  if (CONNECT_TO_NET_SELECT_ETHERNET == eg118_connect_mode) 
  {
    eg118start_wificonnect();
    eg118start_ethconnect();

  } 
  else 
  {
    eg118start_ethconnect();
    eg118start_wificonnect();
  }
 
#ifdef WEBserverSAMPLE_ENABLE
  if (MDNS.begin("esp32")) 
  {
    
  }
  configcallback();
  AsyncWeb_server_instance.begin();

  //Serial.println(" line 1598 AsyncWeb_server started");

#endif
  collect_system_info();

  //writebase_mac();
  getbase_macstring_writetoesp32chip();
  
  response_newcommandresult("USR-EG118", "power on\n");


  /////////////////////////按键begin///////////////////////////////////



  OneButton_buttonreload.setPressMs(1500);

  OneButton_buttonreload.attachClick(OneButton_buttonreloadClick);
  OneButton_buttonreload.attachLongPressStop(OneButton_buttonreloadlongpressup);
//OneButton_buttonreload.attachPressStart(OneButton_buttonreloadpressstart);
#if 1
  //Serial.println("[setup]========================LINE 2913 ->xTaskCreate xTask_watchdog prio  2");
  xTaskCreate(
    xTask_watchdog,   /* 任务函数. */
    "xTask_watchdog", /* 名称 */
    4096,             /* 堆栈大小. */
    NULL,             /* 参数输入传递给任务的*/
    2,                /* 任务的优先级*/
    NULL);            /* 任务所在核心 */
#endif


#ifdef WIFI_DTU_ENABLE
  if (DEVICE_MODE_TCP_DTU_ONLY == eg118_device_mode)
  dtu_instance.tcp_dtu_start();

#endif  //WIFI_DTU_ENABLE

#ifdef MQTT_DTU_ENABLE
if (DEVICE_MODE_MQTT_DTU_ONLY == eg118_device_mode)
{
  dtu_instance.mqtt_dtu_start();
}

#endif

#ifdef HUAWEI_CLOUD_IOT_WITH_NO_TLS
  huaweicloud_instance.start();
#endif


#ifdef TH485_PE_DEMO_ENABLE

  if (DEVICE_MODE_TH485PE_DEMO == eg118_device_mode) 
  {
    Serial.println("[setup]========================LINE 2330->xTaskCreate xTask_TH485_PE_DEMO_func prio  3");
    xTaskCreate(
      xTask_TH485_PE_DEMO_func,   /* Task function. */
      "xTask_TH485_PE_DEMO_func", /* String with name of task. */
      4096,                       /* Stack size in bytes. */
      NULL,                       /* Parameter passed as input of the task */
      3,                          /* Priority of the task.(configMAX_PRIORITIES - 1 being the highest, and 0 being the lowest.) */
      NULL);
  }
#endif



  //临时调试改的
  if ( (DEVICE_Mode_AT_COMMAND == eg118_device_mode) || (DEVICE_Mode_AT_COMMAND_FALSAH_DEFAULT == eg118_device_mode) )
{
  at.start();
}



#ifdef PCMODBUSTOOLDEMO_ENABLE
  if (DEVICE_MODE_MODBUS_RTU == eg118_device_mode) 
  {
      modbus_demo_instance.rtu_start();
  }
    if (DEVICE_MODE_MODBUS_TCP == eg118_device_mode) 
  {
      modbus_demo_instance.tcp_start();
  }
#endif

  //usr_io.init();
  //(DEVICE_MODE_AWS_ONLY == eg118_device_mode)


 #ifdef LOG_ENABLE
  Serial.println("[setup]=================LINE 2609-> end\n");
  #endif
}  //setup

#ifdef WEBserverSAMPLE_ENABLE
static void printwebserveripaddress(void) {


  Serial.println();
  Serial.print("[printwebserveripaddress]=================LINE 2586-> wifi IP address: ");
  if (eg118_wifi_mode == 0) {
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println(WiFi.localIP());
  }

}  //printwebserveripaddress
#endif

short int msgnumber = 0;



//char gv_paraarray[50];









static String comdata = "";


void irqtesthandle2(void) {
  eg118_drvout.M100DOU1_OFF();
  eg118_drvout.M100DOU2_OFF();
  eg118_drvout.M100DOU1_ON();
  eg118_drvout.M100DOU2_ON();
}

void irqtesthandle(void) {
  static char led = 0;
  if (led) {
    led = 0;
  } else {
    led = 1;
  }
  if (0 == led) {
    eg118_drvout.M100DOU1_OFF();
    eg118_drvout.M100DOU2_ON();
  }
  if (1 == led) {
    eg118_drvout.M100DOU1_ON();
    eg118_drvout.M100DOU2_OFF();
  }
}











#ifdef MQTT_DTU_ENABLE

#endif





static void ereasecustom_info(void) 
{
  //清除用户模式，不要透传模式,默认串口at命令
  //默认ap模式
  // refreshEEPROMRAM();
  system_parameter_eg118_main_moudle.set_wifi_status(EEPROM_WIFI_AP_MODE);

  system_parameter_eg118_main_moudle.set_deviceMode_AT_MODE();
  //EEPROM_setdCONNECT_TO_NET_WITH_WIFIMODE();
  system_parameter_eg118_main_moudle.set_CONNECT_TO_NET_WITH_WIFIMODE();
  //随便把dout灯灭了。
  eg118_drvout.M100DOU1_OFF();
  eg118_drvout.M100DOU2_OFF();
  //update_RAM_toEEPROM();
}




//}

long lastMsg = 0;
int value = 0;
char msg[50];
#if 0
void debug_wifi_information(void) {
  switch (eg118_wifi_mode) {
    case EEPROM_WIFI_AP_MODE:
      Serial.printf("[debug_wifi_information]  line 2566->gv_wifi_mode->WIFI_AP_MODE   \n");
      break;
    case EEPROM_WIFI_STA_MODE:
      Serial.printf("[debug_wifi_information  line 2856]->gv_wifi_mode->WIFI_STA_MODE   \n");
      break;
    default:
      Serial.printf("[debug_wifi_information  line 2859]->gv_wifi_mode->unknow   \n");
      break;
  }
}
#endif

#if 0
void debug_device_information(void) {
  switch (eg118_device_mode) {
    case DEVICE_Mode_AT_COMMAND:
      Serial.printf("[loop->debug_device_information line  3167]->eg118_device_mode->DEVICE_Mode_AT_COMMAND=:\n");
      break;
    case DEVICE_MODE_NORMAL:
      Serial.printf("[loop->debug_device_information line line 3170]->eg118_device_mode->DEVICE_MODE_NORMAL=:\n");
      break;
    case DEVICE_MODE_AWS_ONLY:
      Serial.printf("[loop->debug_device_information line  line 3173]->eg118_device_mode->DEVICE_MODE_AWS_ONLY=:\n");
      break;
    case DEVICE_MODE_MODBUS_ONLY:
      Serial.printf("[loop->debug_device_information line 3176]->eg118_device_mode->DEVICE_MODE_MODBUS_ONLY=:\n");
      break;
    case DEVICE_MODE_TCP_DTU_ONLY:
      Serial.printf("[loop]->eg118_device_mode->DEVICE_MODE_TCP_DTU_ONLY=:\n");
      break;
    case DEVICE_MODE_MQTT_DTU_ONLY:
      Serial.printf("[loop]->eg118_device_mode->DEVICE_MODE_MQTT_DTU_ONLY=:\n");
      break;
    case DEVICE_Mode_IDLE:
      Serial.printf("[loop line 3187]->eg118_device_mode->DEVICE_Mode_IDLE\n");
      break;
    case DEVICE_MODE_MODBUSCONTROLIO:
      Serial.printf("[loop line 3190]->eg118_device_mode->DEVICE_MODE_MODBUSCONTROLIO\n");
      break;
    default:
      Serial.printf("[loop line 3193]->eg118_device_mode->unknow\n");
      break;
  }
}
#endif

int count_num = 0;
int count_num1 = 0;
char flag = 0;


void loop(void) 
{
      float usrio_ai1_stat=-1;
  float usrio_ai2_stat=-1;
  float usrio_ai3_stat=-1;
  float usrio_ai4_stat=-1;
  int eg118_di1=-1;//eg118本身主机的di1状态
  char usrio_di1_stat=-1;
  char usrio_di2_stat=-1;
  char usrio_di3_stat=-1;
  char usrio_di4_stat=-1;
    float eg118_ai1_value;
    char eg118adcstring[50];
    uint16_t adcvalue;


#if 1
  usrio_ai1_stat = usr_io_instance.ai_get(1, 1);
  usrio_ai2_stat = usr_io_instance.ai_get(1, 2);
  usrio_ai3_stat = usr_io_instance.ai_get(1, 3);
  usrio_ai4_stat = usr_io_instance.ai_get(1, 4);

  usrio_di1_stat = usr_io_instance.di_get(2, 1);
  usrio_di2_stat = usr_io_instance.di_get(2, 2);
  usrio_di3_stat = usr_io_instance.di_get(2, 3);
  usrio_di4_stat = usr_io_instance.di_get(2, 4);
#endif
 pinMode(39, INPUT);
  snprintf(EG118_di_1_info, 20, " eg118 DI1 is %d . \n", digitalRead(39));      
  snprintf(usrio_ai_1_info, USRIO_AI_INFO_BUF_SIZE, " [slot1] AI1 is %2.0f mA.\n", usrio_ai1_stat/1000);
  snprintf(usrio_ai_2_info, USRIO_AI_INFO_BUF_SIZE, " [slot1] AI2 is %2.0f mA.\n", usrio_ai2_stat/1000);
  snprintf(usrio_ai_3_info, USRIO_AI_INFO_BUF_SIZE, " [slot1] AI3 is %2.0f mA.\n", usrio_ai3_stat/1000);
  snprintf(usrio_ai_4_info, USRIO_AI_INFO_BUF_SIZE, " [slot1] AI4 is %2.0f mA.\n", usrio_ai4_stat/1000);
  memset(usrio_di_1_info,0,USRIO_DI_INFO_BUF_SIZE);
  snprintf(usrio_di_1_info, USRIO_DI_INFO_BUF_SIZE, "[slot2] DI1 is %d . \n", usrio_di1_stat);
  snprintf(usrio_di_2_info, USRIO_DI_INFO_BUF_SIZE, "[slot2] DI2 is %d . \n", usrio_di2_stat);
  snprintf(usrio_di_3_info, USRIO_DI_INFO_BUF_SIZE, "[slot2] DI3 is %d . \n", usrio_di3_stat);
  snprintf(usrio_di_4_info, USRIO_DI_INFO_BUF_SIZE, "[slot2] DI4 is %d . \n", usrio_di4_stat);

snprintf(usrio_ai_info, USRIO_ALL_AI_INFO_BUF_SIZE, "[usrioconfig:slot 1 type->0x%x]usrio AI status:\n----%s----%s----%s----%s----END\n",eg118_slot1_usriotype,usrio_ai_1_info,usrio_ai_2_info,usrio_ai_3_info,usrio_ai_4_info);
memset(usrio_di_info,0,USRIO_ALL_DI_INFO_BUF_SIZE);
snprintf(usrio_di_info, USRIO_ALL_DI_INFO_BUF_SIZE, "[usrioconfig:slot 2 type->0x%x]usrio DI status:\n----%s----%s----%s----%s----END\n",eg118_slot2_usriotype,usrio_di_1_info,usrio_di_2_info,usrio_di_3_info,usrio_di_4_info);
//Serial.printf("[loop line 2881]usrio_di1_stat->%d\n",usrio_di1_stat);
//Serial.printf("[loop line 2880]->%s",usrio_di_1_info);

//response_newcommandresult("[loop]",usrio_di_1_info);
reload_dog();

//eg118_adc.fillraw_adc_value_to_adcbuf();
//delay(500);
eg118_adc.fillraw_adc_value_to_adcbuf();
adcvalue = eg118_adc.get_average_adcvalue();
eg118_ai1_value = eg118_adc.get_ADCchangetocurrent(adcvalue);
//Serial.printf("[loop line 2886]->adcvalue=%d\n",adcvalue);
snprintf(eg118adcstring, 50, " adcvalue is %d . \n", adcvalue);
snprintf(EG118_ai_1_info, EG118_AI_INFO_BUF_SIZE, " eg118 AI1 is %2.0f mA. \n", eg118_ai1_value);
//response_newcommandresult("[loop]",eg118adcstring); 
//Serial.printf("[loop line 2886]->%s.\n",EG118_ai_1_info);
//response_newcommandresult("[loop]",EG118_ai_1_info);
delay(1000);
}  //loop
