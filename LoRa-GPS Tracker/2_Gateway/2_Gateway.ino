#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

#define SSID1 "DimiFi 2G1"
#define PASS1 "newdimigo"
#define SSID2 "DimiFi 2G2"
#define PASS2 "newdimigo"

#define MAX_TX_SIZE 57
#define BC_ADDH 0xFF
#define BC_ADDL 0xFF
#define BC_CHAN 0x0E

const uint8_t M0_PIN = 16;
const uint8_t M1_PIN = 14;
const uint8_t AUX_PIN = 15;
const uint8_t SOFT_RX = 12;
const uint8_t SOFT_TX = 13;

struct CFGstruct {  // settings parameter -> E32 pdf p.28
  uint8_t HEAD = 0xC0;  // do not save parameters when power-down
  uint8_t ADDH = 0x05;
  uint8_t ADDL = 0x01;
  // uint8_t SPED = 0x18;  // 8N1, 9600bps, 0.3k air rate
  // uint8_t SPED = 0x19;  // 8N1, 9600bps, 1.2k air rate
  uint8_t SPED = 0x1A;  // 8N1, 9600bps, 2.4k air rate
  uint8_t CHAN = 0x1F;
  uint8_t OPTION_bits = 0xC4;  // 1, 1, 000, 1, 00
};
struct CFGstruct CFG;


// http://tracker.iwinv.net/upload?gps_long=-1&gps_lati=-1&rssi=404&passcode=4660
const String link = "http://tracker.iwinv.net/upload?passcode=4660";

ESP8266WiFiMulti WiFiMulti;
HTTPClient http;
SoftwareSerial E32(SOFT_RX, SOFT_TX);  // RX, TX

int8_t WaitAUX_H();  // wait till AUX goes high
void SwitchMode(uint8_t mode);  // change mode
void blinkLED();
void triple_cmd(uint8_t Tcmd);  // send 3x Tcmd
void ReceiveMsg();
int8_t SendMsg(String msg);


void setup(){
	Serial.begin(115200);
  E32.begin(9600);
	Serial.println("\r\nInitializing...");
	Serial.println("Device: Gateway \n");

  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(AUX_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  delay(200);

	triple_cmd(0xC4);  // 0xC4: reset
	delay(50);
	int8_t setup_delay = 4;

	WiFi.mode(WIFI_STA);
	WiFiMulti.addAP(SSID1, PASS1);
	WiFiMulti.addAP(SSID2, PASS2);
	while(WiFiMulti.run() != WL_CONNECTED){
		ESP.wdtFeed();
		Serial.print(".");
		delay(250);
		setup_delay--;
	}
	
	Serial.print("\nWiFi Connected: ");
	Serial.println(WiFi.SSID());
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	setup_delay = setup_delay<=0 ? -setup_delay : setup_delay;  // minimize LoRa init delay
	delay(setup_delay*250);

	SwitchMode(3);  // sleep mode/parameter setting
  E32.write((const uint8_t *)&CFG, 6);  // 6 for 6 variables in CFG
  delay(1200);
	SwitchMode(0);

  Serial.println("\nInit complete\n");
}

void loop(){
	float gps_lati=-12, gps_long=-12;
	uint16_t rssi=404;

	ReceiveMsg();
	delay(500);

	String link1 = link+"&gps_lati="+String(gps_lati)+"&gps_long="+String(gps_long)+"&rssi="+String(rssi);
	Serial.print("LINK: ");
	Serial.println(link1);

	http.begin(link1);
	if (http.GET() == HTTP_CODE_OK) {
		String payload = http.getString();
		Serial.print("Response: ");
		Serial.println(payload);
		Serial.println("HTTP upload success");
	}else{
		Serial.println("HTTP Connection Error");
		Serial.println();
	}
	http.end();

  while(1){
		ESP.wdtFeed();
  }
}


void blinkLED(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(75);
  digitalWrite(LED_BUILTIN, LOW);
  delay(75);
}

int8_t WaitAUX_H(){
  uint8_t cnt = 0;

  while((digitalRead(AUX_PIN)==LOW) && (cnt++<15)){
    Serial.print(".");
    delay(100);
  }

  if(cnt>=15){
    Serial.println("  AUX-TimeOut");
    return -1;
  }
  return 0;
}

void SwitchMode(uint8_t mode){
  WaitAUX_H();

  switch (mode){
    case 0:
      // Mode 0 | normal operation
      digitalWrite(M0_PIN, LOW);
      digitalWrite(M1_PIN, LOW);
      break;
    case 1:
      // Mode 1 | wake-up
      digitalWrite(M0_PIN, HIGH);
      digitalWrite(M1_PIN, LOW);
      break;
    case 2:
      // Mode 2 | power save
      digitalWrite(M0_PIN, LOW);
      digitalWrite(M1_PIN, HIGH);
      break;
    case 3:
      // Mode 3 | sleep mode/parammeter setting
      digitalWrite(M0_PIN, HIGH);
      digitalWrite(M1_PIN, HIGH);
      break;
    default:
      return;
  }

  WaitAUX_H();
  delay(10);
}

void triple_cmd(uint8_t Tcmd){
  WaitAUX_H();
  uint8_t CMD[3] = {Tcmd, Tcmd, Tcmd};
  E32.write(CMD, 3);
  Serial.print("Command: ");
  Serial.print(Tcmd, HEX);
  Serial.print(Tcmd, HEX);
  Serial.print(Tcmd, HEX);
  Serial.println();
  delay(15);
}

void ReceiveMsg(){
  if(E32.available()==0){
    return;
  }
  uint8_t data_len = E32.available();
  uint8_t idx;
  blinkLED();

  Serial.print("LoRa Received: [");
  Serial.print(String(data_len));
  Serial.println("] bytes.");

  char RX_buf[data_len+1];
  for(idx=0;idx<data_len;idx++){
    RX_buf[idx] = E32.read();
  }
  // RX_buf[data_len] = "\0";  // NULL terminate array

  Serial.print("data: [");
  Serial.print(RX_buf);
  Serial.println("]");
  Serial.println();
  Serial.flush();

  return;
}

int8_t SendMsg(String msg){
  Serial.print("LoRa transmitting [");
  Serial.print(String(msg.length()));
  Serial.println("] bytes");
  // Serial.print("data: ["); Serial.print(msg); Serial.println("]");

  char text[MAX_TX_SIZE+3];
  msg = msg.substring(0, MAX_TX_SIZE-1);
  if(msg.length() > MAX_TX_SIZE){
    msg.toCharArray(text, MAX_TX_SIZE);
  }else{
    msg.toCharArray(text, msg.length());
  }

  if(CFG.CHAN != 0x0E){  // fixed transmission mode: set ADDH, ADDL, CHAN for first 3 bits
    for(uint8_t q=60; q>=3; q--){
      text[q] = text[q-3];
    }
    text[0] = CFG.ADDH;
    text[1] = CFG.ADDL;
    text[2] = CFG.CHAN;
  }

  SwitchMode(1);  // Wake-up mode
  WaitAUX_H();

  E32.write(text, msg.length()+3);
  WaitAUX_H();
  delay(10);
  
  SwitchMode(0);  // Normal mode
  WaitAUX_H();

  return 0;
}
