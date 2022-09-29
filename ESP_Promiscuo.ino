#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include <sstream>

#define LED_GPIO_PIN                     5
#define WIFI_CHANNEL_SWITCH_INTERVAL  (1000)
#define WIFI_CHANNEL_MAX               (13)

uint8_t mac_lib[100][6];
int mac_count = 0;
int data_count=0;
int mgmnt_count=0;
int pot_prom_m=0;
int pot_prom_d=0;

#define RXD2 16
#define TXD2 17

String mad="";
uint8_t level = 0, channel = 1;

static wifi_country_t wifi_country = {.cc="CO", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
  return ESP_OK;
}

byte mac[6];

void wifi_sniffer_init(void)
{
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
  ESP_ERROR_CHECK( esp_wifi_start() );
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel)
{
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}


const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch(type) {
  case WIFI_PKT_MGMT: return "M";
  case WIFI_PKT_DATA: return "D";
  default:  
  case WIFI_PKT_MISC: return "E";
  }
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  
//  if (type != WIFI_PKT_MGMT)   //filtro por tipo de paquete
//    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

if (check_mac_only(hdr->addr2)){
 printf(",,,,,""%s,""%02x:%02x:%02x:%02x:%02x:%02x," "%02d,""%02d \n",
          wifi_sniffer_packet_type2str(type),
          hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
          hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
          ppkt->rx_ctrl.rssi,
          channel         
          );
          
  Serial2.printf(",,,,,""%s,""%02x:%02x:%02x:%02x:%02x:%02x," "%02d,""%02d \n",
          wifi_sniffer_packet_type2str(type),
          hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
          hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
          ppkt->rx_ctrl.rssi,
          channel         
          );


          
       if (type == WIFI_PKT_MGMT){
            mgmnt_count+=1;
            pot_prom_m+=ppkt->rx_ctrl.rssi;}
       else{
            data_count+=1;
            pot_prom_d+=ppkt->rx_ctrl.rssi;
       }
   }

   
          delay(500);
          return;

}

String create_data_str(int mac_count,int data_count,int mgmnt_count,int pot_prom_m,int pot_prom_d)
{
  String data = "";
  data = String(mac_count) + "," + String(data_count)+ "," +String(mgmnt_count)+","+ String(pot_prom_m/mgmnt_count)+ "," + String(pot_prom_d/data_count );
  return data;
}

int check_mac_only(const uint8_t addr2[6])
{
  for (char i = 0; i < mac_count; i++)
  {
    bool flag = true;
    for (char j = 0; j < 6; j++)
    {
      if (mac_lib[i][j] != addr2[j])
      {
        flag = false;
        break;
      }
    }
    if (flag == true)
      return 0;
  }
  for (char j = 0; j < 6; j++)
  {
    mac_lib[mac_count][j] = addr2[j];
  }
  mac_count++;
  return 1;
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 5 as an output.
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(10);
  wifi_sniffer_init();
  pinMode(LED_GPIO_PIN, OUTPUT);
  
}

// the loop function runs over and over again forever
void loop() {
   delay(2000); // wait for a second
  
  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
  wifi_sniffer_set_channel(channel);
  channel = (channel % WIFI_CHANNEL_MAX) + 1;
 if (channel == 1)
  {
    String data = create_data_str(mac_count,data_count,mgmnt_count,pot_prom_m,pot_prom_d);
    Serial.println(data);
    Serial2.println(data);
    mac_count = 0;
    data_count=0;
    mgmnt_count=0;
    pot_prom_m=0;
    pot_prom_d=0;
  }
}
