/*
 Command scheme:
 #<command char> <arg0> <arg1>....$
 Example:
 #c 15 0 0$
 
 Command list:
 
 [ON/OFF/AUTO]
 #s <0:OFF/1:ON/2:AUTO>
 Example(turn ON):
 #s 1$
 
 [SET COLOR]
 #c <R> <G> <B>
 Example(Red light):
 #c 15 0 0$
 
 [GET STATUS]
 #i$
 Returns:
 >ok <ON/OFF/AUTO>,<SENSOR LEVEL>,<R>,<G>,<B>
 Example:
 HAD:ok 1,0,15,0,0
 
 */
#include <Boards.h>
#include <stdio.h>
#include <stdarg.h>
#include <DHT.h>

#define LED_LEVELS 32
#define CMD_LEN_MAX 32

//pin define
#define PIN_DHT11 2 //IN digital
#define PIN_ONBOARDLED 13 //OUT PWM
#define PIN_3WCREE 9 //OUT PWM
#define PIN_PHOTORES A0 //IN analog
#define PIN_PIR 7 //IN digital

enum LMODE {
  LMODE_OFF = 0,
  LMODE_ON, //manual ON, 100% brightness
  LMODE_AUTO,  
  LMODE_ALARM, //100% brightness blinking
  LMODE_FULL_MANUAL //fully manual, mainly for testing
};

#define LOOP_DELAY_MS (20)
#define PIR_LOCK_PERIOD  (1000) // 1000ms / 20ms * 20
#define PS_LOW_WM (200)
#define PS_HIGH_WM (400)

const unsigned char tbl_cree32[LED_LEVELS] = {0,8,16,25,33,41,49,58,66,74,82,90,99,107,115,123,132,140,148,156,165,173,181,189,197,206,214,222,230,239,247,255};


LMODE g_mode = LMODE_AUTO; //Lighting MODE
unsigned char vr = 31; //LED brightness level

float clr = 0; //LED brightness: current (raw)
float tlr = 0; //LED brightness: target (raw)

unsigned int pir_lock_remain = 0;

char cmdBuffer[CMD_LEN_MAX] = {0};
int cmdLen = 0;

//create dht
DHT dht(PIN_DHT11, DHT11);

//utility functions
void hs_printf(char *fmt, ... ){
  char tmp[64]; // resulting string limited to 64 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, 64, fmt, args);
  va_end (args);
  Serial.print(tmp);
}

//serial command processing functions
void clearCmd(){
  memset(cmdBuffer,0,CMD_LEN_MAX);
  cmdLen = 0;
}

char * getCmd(){
  char tmp;
  tmp = Serial.read();
  while (tmp != -1 && cmdLen < CMD_LEN_MAX) {
    cmdBuffer[cmdLen++] = tmp;
    if (tmp == '$'){
      break;
    }   
    tmp = Serial.read();
  }
  if(cmdBuffer[cmdLen-1] == '$' || cmdLen == CMD_LEN_MAX){
    cmdBuffer[cmdLen-1] = 0;
    if(cmdBuffer[0] == '#' &&
      cmdBuffer[2] == ' ') {
      return cmdBuffer + 1;         
    }
    else {
      clearCmd();
      return 0;
    }
  }
  else {
    return 0;
  } 
}

void processCmd (char * cmd){
  unsigned char s,r,g,b;
  switch (cmd[0]){
  case 'm':
    sscanf(cmd, "m %u", &s);
    g_mode = (LMODE)s;
    break;
  case 'c':
    sscanf(cmd, "c %u %u %u", &r, &g, &b);
    //update(vs, r, g, b); 
    break;
  case 'i':
    print_info(); 
    break;

  default:
    break;
  }
  clearCmd();
  
}

//sensor checker
// return: 0 -> inactive, 1 -> active
int check_pir() {
  if(digitalRead(PIN_PIR) == HIGH) {
    pir_lock_remain = PIR_LOCK_PERIOD;
  }  
  else {
    if (pir_lock_remain == 0) {
      return 0;
    }
    else {
      pir_lock_remain--;
    }
  } 
  return 1; 
}

// return 0 -> dark, 1 -> light
int check_photores() {
  int v_raw = analogRead(PIN_PHOTORES);
  static bool low = false;
  if(v_raw<PS_LOW_WM) {
    low = true;
    return 0;
  }
  else if(v_raw<PS_HIGH_WM){
    if(low == true){
      return 0;
    }
  }
  else if(v_raw>=PS_HIGH_WM){
    low = false;
  }
  return 1;
}

//led control functions
void light_update(){
  if(tlr==clr){
    return;
  }
  if(tlr>clr+1){
    clr+=(tlr-clr)*0.05f;
  }
  else if(tlr>clr){
    clr=tlr;
  }
  else if (tlr<clr-1){
    clr-=(clr-tlr)*0.05f;
  } 
  else if (tlr<clr){
    clr=tlr; 
  } 
  analogWrite(PIN_3WCREE, (int)clr);
}

void light_set_brightness(unsigned int lvl) {
  if (lvl >= LED_LEVELS ) lvl = LED_LEVELS - 1;
  tlr = (float)tbl_cree32[lvl];
}


void print_info(){
  int vphoto = analogRead(PIN_PHOTORES);
  float celsius = dht.readTemperature(); 
  bool pir = digitalRead(PIN_PIR) ;
  hs_printf("ok %u,%u,%u,%u\n",g_mode,pir,vphoto,(int)celsius*10);
}
//void update(unsigned char s,unsigned char r,unsigned char g,unsigned char b){
//
//  int v_raw;
//  
//  if (r >= LED_LEVELS ) r = LED_LEVELS - 1;
//  if (g >= LED_LEVELS ) g = LED_LEVELS - 1;
//  if (b >= LED_LEVELS ) b = LED_LEVELS - 1;
//  if (s >= 3 ) s = 2;
//
//  vs = s;
//  vr = r;
//  vg = g;
//  vb = b;
//
//  if((vs == 1) || (vs == 2 && slvl == HIGH)) {
//    tlr = lvlR[vr];   
//    analogWrite(ledG, lvlG[vg]);     
//    analogWrite(ledB, lvlB[vb]);
//    digitalWrite(OnboardLEDPin, HIGH); 
//  }
//  else{
//    tlr = lvlR[5];  
//    analogWrite(ledG, lvlG[0]);     
//    analogWrite(ledB, lvlB[0]);
//    digitalWrite(OnboardLEDPin, LOW); 
//  }
//  
//  v_raw = analogRead(PhotoResPin);
//  float celsius = dht.readTemperature();
//  
//  hs_printf("HAD:ok %u,%u,%u,%u,%u,%u,%u\n",s,slvl,r,g,b,v_raw, (int)celsius*10);
//}

void setup()  
{
  int i;
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  dht.begin();

  //set pin modes
  pinMode(PIN_PIR, INPUT); 
  pinMode(PIN_3WCREE, OUTPUT); 
  pinMode(PIN_ONBOARDLED, OUTPUT);   
  
  digitalWrite(PIN_ONBOARDLED, LOW);  
  analogWrite(PIN_3WCREE, 0);  
  
  // test CREE
  for(i=0;i<32;i++){
    analogWrite(PIN_3WCREE, tbl_cree32[i]);   
    delay(10); 
  }
  for(i=31;i>=0;i--){
    analogWrite(PIN_3WCREE, tbl_cree32[i]);   
    delay(10); 
  }
  print_info();
 
}

void loop() // run over and over
{
  char * ssCmd;
  static int alarmv = 0;
  
  //check command
  ssCmd = getCmd();
  if (ssCmd != 0) {
    processCmd(ssCmd);
  }
  
  switch(g_mode) {
    case LMODE_OFF:
    light_set_brightness(0);    
    break;

    case LMODE_ON:
    light_set_brightness(LED_LEVELS-1);    
    break; 
    
    case LMODE_ALARM:
    if(alarmv>10){
      analogWrite(PIN_3WCREE, 255);
      alarmv=0;
    }
    else{
      analogWrite(PIN_3WCREE, 0);
      alarmv++;
    }
    break;

    case LMODE_AUTO:
      if(!check_photores()){
        if(check_pir()){
          light_set_brightness(LED_LEVELS-1);   
        }
        else{  
          light_set_brightness(3);
        }
      }
      else{
        light_set_brightness(0);
      }
    break;
  }
  light_update();
  delay(20); 
}

