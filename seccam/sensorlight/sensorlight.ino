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

#include <stdarg.h>
#include <stdio.h>
#include <DHT.h>
#include <Boards.h>


#define LED_LEVELS 32
#define CMD_LEN_MAX 32


//pin mapping
#define DHT11Pin 2
#define OnboardLEDPin 13
#define PhotoResPin A0


DHT dht(DHT11Pin, DHT11);

int ledR = 9;  //PWM
int ledG = 5;  //PWM  
int ledB = 3;  //PWM
int sensor = 7;

unsigned char vs = 2;
unsigned char vr = 31;
unsigned char vg = 31;
unsigned char vb = 31;

unsigned char slvl = -1;


unsigned char clr = 0,clg = 0,clb = 0;
unsigned char tlr = 0,tlg = 0,tlb = 0;

const unsigned char lvlR[LED_LEVELS] = {
  0,8,16,25,33,41,49,58,66,74,82,90,99,107,115,123,132,140,148,156,165,173,181,189,197,206,214,222,230,239,247,255};
const unsigned char lvlG[LED_LEVELS] = {
  0,2,3,5,6,8,10,11,13,15,16,18,19,21,23,24,26,27,29,31,32,34,35,37,39,40,42,44,45,47,48,50};
const unsigned char lvlB[LED_LEVELS] = {
  0,2,5,7,9,11,14,16,18,20,23,25,27,29,32,34,36,38,41,43,45,47,50,52,54,56,59,61,63,65,68,70};
int fadeValue = 0;

char cmdBuffer[CMD_LEN_MAX] = {
  0};
int cmdLen = 0;


void fadeled(){
  if(tlr>clr){
    clr+=1;
    analogWrite(ledR, clr);  
  }
  else {
    clr-=1;
    analogWrite(ledR, clr);     
  } 
}


void hs_printf(char *fmt, ... ){
  char tmp[64]; // resulting string limited to 64 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, 64, fmt, args);
  va_end (args);
  Serial.print(tmp);
}

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
      hs_printf("HAD:bad command: %s\n", cmdBuffer);
      clearCmd();
      return 0;
    }
  }
  else {
    return 0;
  } 
}

void update(unsigned char s,unsigned char r,unsigned char g,unsigned char b){

  int v_raw;
  
  if (r >= LED_LEVELS ) r = LED_LEVELS - 1;
  if (g >= LED_LEVELS ) g = LED_LEVELS - 1;
  if (b >= LED_LEVELS ) b = LED_LEVELS - 1;
  if (s >= 3 ) s = 2;

  vs = s;
  vr = r;
  vg = g;
  vb = b;

  if((vs == 1) || (vs == 2 && slvl == HIGH)) {
    tlr = lvlR[vr];   
    analogWrite(ledG, lvlG[vg]);     
    analogWrite(ledB, lvlB[vb]);
    digitalWrite(OnboardLEDPin, HIGH); 
  }
  else{
    tlr = lvlR[5];  
    analogWrite(ledG, lvlG[0]);     
    analogWrite(ledB, lvlB[0]);
    digitalWrite(OnboardLEDPin, LOW); 
  }
  
  v_raw = analogRead(PhotoResPin);
  float celsius = dht.readTemperature();
  
  hs_printf("HAD:ok %u,%u,%u,%u,%u,%u,%u\n",s,slvl,r,g,b,v_raw, (int)celsius*10);
}

void processCmd (char * cmd){
  unsigned char s,r,g,b;

  hs_printf("command: %s\n", cmd);

  switch (cmd[0]){
  case 's':
    sscanf(cmd, "s %u", &s);
    update(s, vr, vg, vb);
    break;
  case 'c':
    sscanf(cmd, "c %u %u %u", &r, &g, &b);
    update(vs, r, g, b); 
    break;
  case 'i':
    update(vs, vr, vg, vb); 
    break;

  default:
    break;
  }
  clearCmd();

}

void setup()  
{
  int i;
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  dht.begin();

  //set pin modes
  pinMode(sensor, INPUT); 
  pinMode(OnboardLEDPin, OUTPUT); 
  digitalWrite(OnboardLEDPin, LOW);  

  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.println("OK");  
  
  // test led
  for(i=0;i<32;i++){
    analogWrite(ledR, lvlR[i]);   
    delay(10); 
  }
  for(i=31;i>=1;i--){
    analogWrite(ledR, lvlR[i]);   
    delay(10); 
  }
  for(i=0;i<32;i++){
    analogWrite(ledG, lvlG[i]);   
    delay(10); 
  }
  for(i=31;i>=1;i--){
    analogWrite(ledG, lvlG[i]);   
    delay(10); 
  }
  for(i=0;i<32;i++){
    analogWrite(ledB, lvlB[i]);   
    delay(10); 
  }
  for(i=31;i>=1;i--){
    analogWrite(ledB, lvlB[i]);   
    delay(10); 
  }
  
  //set initial LED level
  for(i=0;i<vr;i++){
    analogWrite(ledR, lvlR[i]);   
    analogWrite(ledG, lvlG[i]);     
    analogWrite(ledB, lvlB[i]);  
    delay(10); 
  }
}

void loop() // run over and over
{
  char * ssCmd;
  
  //check sensor value
  if(slvl != digitalRead(sensor)){
    slvl = digitalRead(sensor);
    if (vs == 2){
      update(vs, vr, vg, vb);
    }
  }
  
  //check command
  ssCmd = getCmd();
  if (ssCmd != 0) {
    processCmd(ssCmd);
  }
  
  fadeled();
  delay(20); 
}

