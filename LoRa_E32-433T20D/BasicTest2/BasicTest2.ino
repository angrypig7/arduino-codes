#include <SoftwareSerial.h>

#define dev_num 2  // 1: TX, 2: RX
#define MAX_TX_SIZE 57

// Broadcast TX/RX Addr: 0xFFFF / 0x0000, channel 14
#define BC_ADDH 0xFF
#define BC_ADDL 0xFF
#define BC_CHAN 0x0E

// Wemos D1 Mini
// const uint8_t M0_PIN = D0;
// const uint8_t M1_PIN = D1;
// const uint8_t AUX_PIN = D2;
// const uint8_t SOFT_RX = D6;
// const uint8_t SOFT_TX = D7;

// Wemos D1 Mini Pro
// const uint8_t M0_PIN = 16;
// const uint8_t M1_PIN = 14;
// const uint8_t AUX_PIN = 4;
// const uint8_t SOFT_RX = 12;
// const uint8_t SOFT_TX = 13;

// ATMega328p
const uint8_t M0_PIN = 7;
const uint8_t M1_PIN = 8;
const uint8_t AUX_PIN = A0;
const uint8_t SOFT_RX = 10;
const uint8_t SOFT_TX = 11;

struct CFGstruct {  // settings parameter -> E32 pdf p.28
  uint8_t HEAD = 0xC0;  // do not save parameters when power-down
  uint8_t ADDH = 0x05;
  uint8_t ADDL = 0x01;
  // uint8_t SPED = 0x18;  // 8N1, 9600bps, 0.3k air rate
  // uint8_t SPED = 0x19;  // 8N1, 9600bps, 1.2k air rate 
  uint8_t SPED = 0x1A;  // 8N1, 9600bps, 2.4k air rate
  // uint8_t CHAN = 0x10;  // 424Mhz
  uint8_t CHAN = 0x18;  // 434Mhz
  uint8_t OPTION_bits = 0xC4;  // 1, 1, 000, 1, 00
};
struct CFGstruct CFG;
SoftwareSerial E32(SOFT_RX, SOFT_TX);  // RX, TX

bool ReadAUX();  // read AUX logic level
int8_t WaitAUX_H();  // wait till AUX goes high and wait a few millis
void SwitchMode(uint8_t mode);  // change mode to mode
void blinkLED();
void triple_cmd(uint8_t Tcmd);  // send 3x Tcmd
void ReceiveMsg();
int8_t SendMsg(String msg);

// switch(dev_num){
// 	case 1:
// 		break;
// 	case 2:
// 		break;
// 	default:
// 		break;
// }

//=== SETUP =========================================+
void setup(){
  Serial.begin(115200);
  E32.begin(9600);

  Serial.println("\nInitializing...");
  switch(dev_num){
    case 1:
      Serial.println("Device: 1: TX");
      break;
    case 2:
      Serial.println("Device: 2: RX");
      break;

    default:
      Serial.println("Device: UNDEFINED");
      break;
  }

  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(AUX_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  triple_cmd(0xC4);  // 0xC4: reset
  delay(1000);

  SwitchMode(3);  // sleep mode/parameter setting
  E32.write((const uint8_t *)&CFG, 6);  // 6 for 6 variables in CFG
  delay(1200);

  SwitchMode(0);

  Serial.println("Init complete");
}
//=== SETUP =========================================-

//=== LOOP ==========================================+
void loop(){
  switch(dev_num){
    case 1:
      while(1){
        String dataStr = "test data ";
        if(SendMsg(dataStr) == 0){  // success
          blinkLED();
        }

        delay(1200);
      }
      break;

    case 2:
      while(1){
        ReceiveMsg();
        delay(500);
      }
      break;

    default:
      Serial.println("DEVICE NUM ERROR");
      break;
  }
  
}
//=== LOOP ==========================================-


void blinkLED(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(75);
  digitalWrite(LED_BUILTIN, LOW);
  delay(75);
}

//=== AUX ===========================================+
bool ReadAUX(){
  int val = analogRead(AUX_PIN);

  if(val<50){
    return LOW;
  }else{
    return HIGH;
  }
}

int8_t WaitAUX_H(){
  uint8_t cnt = 0;

  while((ReadAUX()==LOW) && (cnt++<100)){
    Serial.print(".");
    delay(100);
  }

  if(cnt>=100){
    Serial.println("  AUX-TimeOut");
    return -1;
  }
  return 0;
}
//=== AUX ===========================================-

//=== Mode Select ===================================+
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
//=== Mode Select ===================================-

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
  RX_buf[data_len] = "\0";  // NULL terminate array

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
