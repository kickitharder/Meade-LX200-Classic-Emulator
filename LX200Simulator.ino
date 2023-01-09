#define version "LX200 Simulator V2.230109#"

#pragma region DEFINITIONS
#include <EEPROM.h>
#include <Arduino.h>
//#include <SoftwareSerial.h>
#define sol2sid   1.0027379093508                 // ~366.25/365.25
#define rad2deg   57.29577951
#define deg2rad   0.017453293
#define TEST      0

//SoftwareSerial Serial0(11, 12);   //RXD, TXD
long objRA  = 0, telRAb4  = 0, telRA  = 0;      // RA has 24*60*60 = 86400 units
long objDEC = 0, telDECb4 = 0, telDEC = 0;      // DEC has 180*60*60 = 648000 units

long targRA = 0, targDEC  = 0;
long telAZ  = 0, telALT   = 0;                  // AZ has 1296000 and Alt has 648000 units
long objAZ  = 0, objALT   = 0;
int  telRAms = 0;
bool prec = 0;                                  // 0 = Low precision, 1 = High precision. For coordinates
byte slewAltAz = 1;                             // 1 = AltAz Slew, 0 = Eq Slew
int freq = 601;                                 // Track frequency x 10
bool freqQ = true;                              // true = sidereal rate, false = custom rate
char homeStat = '0';

unsigned long slewStartTime, slewTimerRA, slewTimerDEC, moveRAmillis, moveDECmillis;
long sidOffset = 0;
int slewRAdir, slewDECdir, moveRAdir, moveDECdir;
int slewRate = 1928;                            // SLEW rate 2-8 deg/s
int moveRate = 1928;                            // 0. NONE 1.GUIDE = 2x sid, 2.CNTR = 32x sid, 3.FIND = 2 degs/s, 4.SLEW = 2-8 degs/s

struct {
  char siteNames[4][4] = {"GRE", 
                          "AAA",
                          "AAA",
                          "AAA"};
  int latitude[4] =  {3089, 0, 0, 0};
  int longitude[4] = {0, 0, 0, 0};
  long homeHA = 0;
  long homeDEC = 0;
  byte siteNo = 0;
  char align = 'P';
  uint8_t hourOffset = 0;
} ee;

int yr = 2022; 
byte mth = 12,  day = 27, hr = 18, min = 38;
unsigned long ms = 33000, clock;
byte higher     = 0, 
     lower      = 90,
     brighter   = 200,
     fainter    = -55,
     larger     = 0,
     smaller    = 200,
     field      = 15,
     catalog   	= 0,        // 0 = NGC, 1 = IC, 2 = UGC
     starList   = 0,        // 0 = alignment stars & planets, 1 = SAO, 2 = GCVS
     object     = 2,        // 0 = Catalog, 1 = Messier, 2 = Star
     qual       = 6;
int cngc       	= 0,        // 1 - 7840
    ic        	= 0,        // 1 - 5386
    ugc       	= 0,        // 1 - 12921
    messier     = 1,        // 1 - 110
    star      	= 0,        // 1 - 351, 901 - 909
    ptr         = 0;

const char quality[7][3] = {"SU", "EX", "VG", "GD", "FR", "PR", "VP"};
const char p1[]  PROGMEM = " MERCURY";
const char p2[]  PROGMEM = "   VENUS";
const char p3[]  PROGMEM = "THE MOON";
const char p4[]  PROGMEM = "    MARS";
const char p5[]  PROGMEM = " JUPITER";
const char p6[]  PROGMEM = "  SATURN";
const char p7[]  PROGMEM = "  URANUS";
const char p8[]  PROGMEM = " NEPTUNE";
const char p9[]  PROGMEM = "   PLUTO";
const char *const messages[] PROGMEM = {p1, p2, p3, p4, p5, p6, p7, p8, p9};
char resp[34], buf[16];
char objTypes[6] = "GPDCO";
int objMag = 0, objSize  = 0;
byte objClass = 0, objQual = 0;
#pragma endregion DEFINITIONS
#pragma region SETUP
//=================================================================================================
// SETUP
//=================================================================================================
void setup() {
  clock = millis();
  if (EEPROM.read(0) == 'Z' || EEPROM.read(0) == 255) EEPROM.put(0, ee);
  EEPROM.get(0, ee);
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  telRA = sidTime();

  objDEC = 320400;
  objRA = normL(telRA + 12 * 3600, 86400);

  buf[0] = 0;
  //Serial.print('\n');
}
#pragma endregion SETUP
#pragma region LOOP
//=================================================================================================
// LOOP
//=================================================================================================
void loop() {
  actionCmd();
  if (slewRAdir || slewDECdir || moveDECdir || moveRAdir) {  // LED is on while slewing is in progress
    if ((millis() < slewTimerRA) || (millis() < slewTimerDEC) || moveDECdir || moveRAdir) digitalWrite(13, HIGH);
  } else digitalWrite(13, LOW);
}
#pragma endregion LOOP
#pragma region COMMAND HANDLING
//=================================================================================================
// COMMAND HANDLING
//=================================================================================================
void actionCmd() {
  char c;
  while (Serial.available()) {
    c = Serial.read();
    if (c == char(6)) c = '6';
    if (c < 32 || c > 127) return;
    if (buf[0] != ':' || ptr >= 15) ptr = 0;
    buf[ptr] = c;
    buf[++ptr] = 0;
    if (c == '#') break;
  }
  if (buf[0] != ':' || c != '#') return;
  ptr = buf[ptr - 1] = resp[0] = resp[1] = 0;   // Remove '#' and reset other things
  memmove(buf, buf + 1, 15);                    // Shift buf chars to the left one.
// HOME SEARCH IN OPERATION
  if (cmd("Q#")) stop();
  if (cmd("hF")) resp[0] = homeStat & '3';
  if (homeStat == '2') return;
// COORDINATES
  if (cmd("GR")) getRA();
  if (cmd("Gr")) getHMS(objRA);
  if (cmd("Sr")) setObjRA();
  if (cmd("GD")) getDEC();
  if (cmd("Gd")) getDMS(objDEC);
  if (cmd("Sd")) setObjDEC();
  if (cmd("GA")) getALT();
  if (cmd("SA")) setObjALT();
  if (cmd("GZ")) getAZ();
  if (cmd("SZ")) setObjAZ();
  if (cmd("U"))  resp[0] = prec++ & 0;
  if (cmd("CM")) matchCoords();
// DATE AND TIME
  if (cmd("GC")) getDate();
  if (cmd("SC")) setDate();
  if (cmd("GL")) getTime24();
  if (cmd("Ga")) getTime12();
  if (cmd("SL")) setTime();
  if (cmd("GS")) getSid();
  if (cmd("SS")) setSid();
  if (cmd("GG")) getTimeOffset();
  if (cmd("SG")) setTimeOffset();
// MOVEMENT
  if (cmd("D"))  distBars();
  if (cmd("Sw")) setSlewRate();
  if (cmd("GT")) sprintf(resp, "%02d.%01d", freq / 10, freq % 10);
  if (cmd("ST")) setFreq();
  if (cmd("TM")) resp[0] = freqQ = false;
  if (cmd("TQ")) resp[0] = !(freqQ = true);
  if (cmd("T+")) resp[0] = ((!freqQ && freq < 601) ? freq++ : 0) & 0;
  if (cmd("T-")) resp[0] = ((!freqQ && freq > 564) ? freq-- : 0) & 0;
  if (buf[0] == 'R') setMoveRate();       // RG RC RM RS#
  if (buf[0] == 'M') move();              // Mn Ms Me Mw MS MA
  if (buf[0] == 'Q') stop();              // Qn Qs Qe Qw Q
  if (cmd("hS")) homeSave();
  if (cmd("hF")) homeSet();
  if (cmd("hP")) homePark();

// SITE DETAILS
  if (cmd("Gt")) getLat();
  if (cmd("St")) setLat();
  if (cmd("Gg")) getLong();
  if (cmd("Sg")) setLong();
  if (buf[0] == 'G') getSiteName();       // GM GN GO GP
  if (buf[0] == 'S') setSiteName();       // SM SN SO SP
  if (buf[0] == 'G') getAlignMenu();      // G0 G1 G2
  if (buf[0] == 'W') if((buf[1] >= '1') && (buf[1] <= '4')) resp[0] = (ee.siteNo = buf[1] - '1') & 0;
// MISCELLANEOUS
  if (cmd("6")) resp[0] = ee.align;
  if (buf[0] == 'A') setAlign();
  if (cmd("VE")) resp[0] = Serial.println(F(version)) & 0;
  if (cmd("VR")) EEPROM.put(0, 'Z');
// LIBRARY
  if (cmd("Gy")) strcpy(resp, objTypes);
  if (cmd("Sy")) setObjType();
  if (cmd("Gq")) strcpy(resp, quality[qual]);
  if (cmd("Sq")) resp[0] = (qual > 6 ? qual = 0 : qual++) & 0;
  if (cmd("Gh")) sprintf(resp, "%02d\xDF", higher);
  if (cmd("Sh")) setHigher();
  if (cmd("Go")) sprintf(resp, "%02d\xDF", lower);
  if (cmd("So")) setLower();
  if (cmd("Gb")) getBrighter();
  if (cmd("Sb")) setBrighter();
  if (cmd("Gf")) getFainter();
  if (cmd("Sf")) setFainter();
  if (cmd("Gl")) sprintf(resp, "%03d'", larger);
  if (cmd("Sl")) setLarger();
  if (cmd("Gs")) sprintf(resp, "%03d'", smaller);
  if (cmd("Ss")) setSmaller();
  if (cmd("GF")) sprintf(resp, "%03d'", field);
  if (cmd("SF")) setField();
  if (cmd("Lf")) sendMsg("Objects:  0                     ");
  if (cmd("LC")) setCatalogObj();
  if (cmd("LM")) setMess();
  if (cmd("LS")) setStar();
  if (cmd("LI")) getInfo();
  if (cmd("Lo")) setCatalog();
  if (cmd("Ls")) setStarType();

/* NO ACTION AND NO REPSONSE COMMANDS:
  Date & Time:    H#
  Miscellaneous:  B+  B-  B0  B1  B2  B3  F+  F-  FQ  FF  FS  r+  r-  f+  f-  hs  hf  hp
  Library:        LF  LN  LB
*/
  sendResp();
}
//-------------------------------------------------------------------------------------------------
bool cmd(char c[]) {
  return (buf[0] == c[0] && buf[1] == c[1]) ? (resp[0] = '0') != 0 : 0; // Black magic
}
//-------------------------------------------------------------------------------------------------
void sendMsg(char* msg) {
  Serial.print(msg);
  Serial.print('#');
  resp[0] = 0;
  //if (TEST) Serial.println();
}
//-------------------------------------------------------------------------------------------------
void sendResp() {
  Serial.print(resp);
  if (resp[1]) Serial.print('#');
  //if (TEST) Serial.println();
}
#pragma endregion COMMAND HANDLING
#pragma region COORDINATES
//=================================================================================================
// COORDINATES
//=================================================================================================
void getRA() {                //  :GR#
  if (ee.align == 'L') {
    prec ? strcpy(resp, "00:00:00") : strcpy(resp, "00:00.0");
    return;
  }
  updateCoords();
  getHMS(telRA);
}
//-------------------------------------------------------------------------------------------------
void getHMS(long coord) {
  int h = coord / 3600L;
  int m = (coord % 3600L) /60;
  int s = coord % 60;
  int t = s / 6;
  if(prec) {
    sprintf(resp, "%02d:%02d:%02d", h, m, s);
  }
  else sprintf(resp, "%02d:%02d.%1d", h, m ,t);
}
//-------------------------------------------------------------------------------------------------
void setObjRA() {             //  :SrHH:MM.T# :SrHH:MM:SS#
  int h, m, t, s;
  if(sscanf(buf, "Sr%02d:%02d.%1d", &h, &m, &t) == 3) {
    if((h < 24) && (m < 60) && (t < 10)) {
      objRA = h * 3600L + m * 60L + t * 6L;
      resp[0] = '1';
    }
  }
  else if(sscanf(buf, "Sr%02d:%02d:%02d", &h, &m, &s) == 3) {
    if((h < 24) && (m < 60) && (s < 60)) {
      objRA = h * 3600L + m * 60L + s;
      resp[0] = '1';
    }
  }
}
//-------------------------------------------------------------------------------------------------
void getDEC() {               //  :GD#
  if (ee.align == 'L') {
    prec ? strcpy(resp, "00\xDF\x30\x30:00") : strcpy(resp, "00\xDF\x30\x30");
      return;
  }
  updateCoords();
  getDMS(telDEC);
}
//-------------------------------------------------------------------------------------------------
void getDMS(long coord) {
  int d, m, s, h = coord < 0;
  coord = abs(coord);
  d = coord / 3600L;
  m = (coord % 3600L) / 60L;
  s = coord % 60L;
  coord = abs(coord);
  if(prec) {
    sprintf(resp, "+%02d\xDF%02d:%02d", d, m, s);
  }
  else {
    coord /= 60;
    sprintf(resp, "+%02d\xDF%02d", d, m);
  }
  if(h) resp[0] = '-';
}
//-------------------------------------------------------------------------------------------------
void setObjDEC() {            //  :SdsDD*MM# :SdsDD*MM:SS#
  int d, m, s;

  if (sscanf(buf, "Sd+%02d\xDF%02d", &d, &m) == 2) setDEC(d, m, 0, 1);
  if (sscanf(buf, "Sd-%02d\xDF%02d", &d, &m) == 2) setDEC(d, m, 0, -1);
  if (sscanf(buf, "Sd+%02d*%02d"   , &d, &m) == 2) setDEC(d, m, 0, 1);
  if (sscanf(buf, "Sd-%02d*%02d"   , &d, &m) == 2) setDEC(d, m, 0, -1);

  if (sscanf(buf, "Sd+%02d\xDF%02d:%02d", &d, &m, &s) == 3) setDEC(d, m, s, 1);
  if (sscanf(buf, "Sd-%02d\xDF%02d:%02d", &d, &m, &s) == 3) setDEC(d, m, s, -1);
  if (sscanf(buf, "Sd+%02d*%02d:%02d"   , &d, &m, &s) == 3) setDEC(d, m, s, 1);
  if (sscanf(buf, "Sd-%02d*%02d:%02d"   , &d, &m, &s) == 3) setDEC(d, m, s, -1);

  // if (resp[0] == '1') slewModeRA = slewModeDEC = 0;
}
//-------------------------------------------------------------------------------------------------
void setDEC(int d, int m, int s, int h) {
  if (d >= 90 || m >= 60 || s >= 60) return;
  objDEC = (3600L * d + 60L * m + s) * h;
  resp[0] = '1';
}
//-------------------------------------------------------------------------------------------------
void getALT() {               //  :GA#
  updateCoords();
  eq2altAz(telRA, telDEC);
  getDMS(telALT);
}
//-------------------------------------------------------------------------------------------------
void setObjALT() {            //  :SAsDD*MM#  :SAsDD*MM:SS#
  int d, m, s;
  if (ee.align == 'P') return;
  if (sscanf(buf, "Sa+%02d\xDF%02d", &d, &m)) setALT(d, m, 0, 1);
  if (sscanf(buf, "Sa-%02d\xDF%02d", &d, &m)) setALT(d, m, 0, -1);
  if (sscanf(buf, "Sa+%02d*%02d"   , &d, &m)) setALT(d, m, 0, 1);
  if (sscanf(buf, "Sa-%02d*%02d"   , &d, &m)) setALT(d, m, 0, -1);

  if (sscanf(buf, "Sa+%02d\xDF%02d:%02d", &d, &m, &s)) setALT(d, m, s, 1);
  if (sscanf(buf, "Sa-%02d\xDF%02d:%02d", &d, &m, &s)) setALT(d, m, s, -1);
  if (sscanf(buf, "Sa+%02d*%02d:%02d"   , &d, &m, &s)) setALT(d, m, s, 1);
  if (sscanf(buf, "Sa-%02d*%02d:%02d"   , &d, &m, &s)) setALT(d, m, s, -1);
}
//-------------------------------------------------------------------------------------------------
void setALT(int d, int m, int s, int h) {
  if (d >= 90 || m >= 60 || s >= 60) return;
  objALT = 1000L * (h * (d * 3600L + min * 60L + ms / 1000L));
  resp[0] = '1';
}
//-------------------------------------------------------------------------------------------------
void getAZ() {                //  :GZ#
  updateCoords();
  eq2altAz(telRA, telDEC);
  long az = telAZ / 1000;
  if(prec) {
    sprintf(resp, "%03d\xDF%02d'%02d", int(az / 3600), int(az / 60) % 60, az % 60);
  }
  else {
    az /= 60;
    sprintf(resp, "%03d\xDF%02d", int(az/ 60), az % 60);
  }
}
//-------------------------------------------------------------------------------------------------
void setObjAZ() {             //  :Szddd*mm#  :Szddd*mm*ss#
  int d, m, s;
  if (ee.align == 'P') return;
  if (sscanf(buf, "Sz%03d\xDF%02d"), &d, &m)          setAZ(d, m, 0);
  if (sscanf(buf, "Sz%03d\xDF%02d:%02d"), &d, &m, &s) setAZ(d, m, s);
  if (sscanf(buf, "Sz%03d*%02d"), &d, &m);            setAZ(d, m, 0);
  if (sscanf(buf, "Sz%03d*%02d:%02d"), &d, &m, &s);   setAZ(d, m, s);
}
//-------------------------------------------------------------------------------------------------
void setAZ(int d, int m, int s) {
  if (d >= 360 || m >= 60 || s >= 60) return;
  objAZ = 1000L * (d * 3600L + m * 60L + s);
  resp[0] = '1';
}
//-------------------------------------------------------------------------------------------------
void matchCoords() {          //  :CM#
  telRA = objRA;
  telDEC = objDEC;
  sendMsg("Coordinates     matched.        "); // Coords matched
}
#pragma endregion COORDINATES
#pragma region DATE AND TIME
//=================================================================================================
// DATE & DATE
//=================================================================================================
void getDate() {              //  :GC#
  updateTime();
  sprintf(resp, "%02d/%02d/%02d", mth, day, yr % 100);
}
//-------------------------------------------------------------------------------------------------
void getTime24() {            //  :GL#
  updateTime();
  sprintf(resp, "%02d:%02d:%02d", hr, min, int(ms / 1000));
}
//-------------------------------------------------------------------------------------------------
void getTime12() {            //  :Ga#
  updateTime();
  sprintf(resp, "%02d:%02d:%02d", hr % 24, min, int(ms / 1000)); // Firmware 3.34 return 24 hour clock!
}
//-------------------------------------------------------------------------------------------------
void getSid() {               //  :GS#
  unsigned long sid = sidTime();
  sprintf(resp, "%02d:%02d:%02d", int(sid / 3600), int((sid / 60) % 60), int(sid % 60));
}
//-------------------------------------------------------------------------------------------------
void setDate() {              //  :SCMM/DD/YY#
  int y, m, d;
  if(sscanf(buf, "SC%02d/%02d/%02d", &m, &d, &y)) {
    if(y > 100) return;
    y += 1900 + (y < 92) * 100;
    if((m > 12)|(m < 1)) return;
    switch (m) {
      case 2:  if(d > (28 + (y % 4 == 0))) return;  //Feb
      case 4:  if(d > 30) return;       //Apr
      case 6:  if(d > 30) return;       //Jun
      case 9:  if(d > 30) return;       //Sep
      case 11: if(d > 30) return;       //Nov
      default: if(d > 31) return;       //Jan, Mar, May, Jul, Aug, Oct or Dec
    }
    updateTime();
    day = d;
    mth = m;
    yr = y;
    resp[0] = '1';
    sendResp();
    sendMsg("Updating        planetary data. ");
    delay(5000);
    for (byte i = 0; i <= 31; i++) resp[i] = ' ';
  }
}
//-------------------------------------------------------------------------------------------------
void setTime() {              //  :SLHH:MM:SS#
  int h, m, s;
  if(sscanf(buf, "SL%02d:%02d:%02d", &h, &m, &s)) {
    if((h < 24) && (m < 60) && (s < 60)) {
      updateTime();
      hr = h;
      min = m;
      ms = s * 1000;
      resp[0] = '1';
      clock = millis();
    }
  }
}
//-------------------------------------------------------------------------------------------------
void setSid() {               //  :SSHH:MM:SS#
  int h, m, s;
  updateTime();
  if(sscanf(buf, "SS%02d:%02d:%02d", &h, &m, &s)) {
    if ((h >= 24) || (m >= 60) || (s >= 60)) return;
    sidOffset = normL((h * 3600 + m * 60 + s) - sidTime(), 84600);
    resp[0] = '1';
  }
}
//-------------------------------------------------------------------------------------------------
void getTimeOffset() {        //  :GG# - Returns sHH#
  sprintf(resp, "+%d", ee.hourOffset);
  if (ee.hourOffset < 0) resp[0] = '-';
}
//-------------------------------------------------------------------------------------------------
void setTimeOffset() {        //  :SGsHH#
  int v;
  if (sscanf(buf, "SG%d", &v));
  if (v < -12 || v > 12) return;
  ee.hourOffset = v;
  resp[0] = '1';
  EEPROM.put(0, ee);
}
#pragma endregion DATE AND TIME
#pragma region MOVEMENT
  //=================================================================================================
// MOVEMENT
//=================================================================================================
void move() {                 // :Mn#  :Ms#  :Me#  :Mw#  :MS#  :MA#
  switch (buf[1]) {
    case 'n': if (moveDECdir != 1)  {updateCoords(); moveDECdir =  1; moveDECmillis = millis();} break;
    case 's': if (moveDECdir != -1) {updateCoords(); moveDECdir = -1; moveDECmillis = millis();} break;
    case 'e': if (moveRAdir != 1)   {updateCoords(); moveRAdir  =  1; moveRAmillis  = millis();} break;
    case 'w': if (moveRAdir != -1)  {updateCoords(); moveRAdir  = -1; moveRAmillis  = millis();} break;
    case 'S': homeStat = '0'; move2Eq(); return;
    case 'A': homeStat = '0'; move2AltAz(); return;
  }
  resp[0] = 0;
}
//-------------------------------------------------------------------------------------------------
void stop() {                 //  :Qn#  :Qs#  :Qe#  :Qw#  :Q#
  resp[0] = 0;
  updateCoords();
   switch (buf[1]) {
    case 'n': moveDECdir = 0; break;
    case 's': moveDECdir = 0; break;
    case 'e': moveRAdir  = 0; break;
    case 'w': moveRAdir  = 0; break;
    case '\0': slewRAdir = slewDECdir = 0; break;
  }
}
//-------------------------------------------------------------------------------------------------
void setSlewRate() {          //  :SwN#
  byte n = buf[2] - '0';
  if((n >= 2) & (n <= 8)) {
    slewRate = n * 241;
    resp[0] = '1';
    if (moveRate > 480) moveRate = slewRate;  // If SLEW speed is set, update the move rate
  }
}
//-------------------------------------------------------------------------------------------------
void setFreq() {              //  :STnn.n#
  int a = 0, b = 0, v;
  if (sscanf(buf, "ST%2d.%1d", &a, &b) < 2) {
    if (!sscanf(buf, "ST%2d", &a)) return;
  }
  v = a * 10 + b;
  if (v < 564 || v > 601) return;
  if (freqQ) return;
  resp[1] = '1';
  freq = v;
}
//-------------------------------------------------------------------------------------------------
void setMoveRate() {          //  :RG# :RC#: :RM# :RS#
  resp[0] = 0;
  switch (buf[1]) {
    case 'G': moveRate = 2; break;                  // These are RA rates - multiplied by 15 for DEC
    case 'C': moveRate = 32; break;
    case 'M': moveRate = 480; break;
    case 'S': moveRate = slewRate; break;
    default:  return;
  }
}
//-------------------------------------------------------------------------------------------------
void distBars() {             //  :D#
  updateCoords();
  int barsRA  = int(abs(telRA - targRA) / 2400L);    // (24*60*60)/(360/10)=2400
  int barsDEC = int(abs(telDEC - targDEC) / 36000L); // (360*60*60)/(360/10)=36000
  if (!barsRA && telRA != targRA) barsRA++;
  if (!barsDEC && telDEC !=targDEC) barsDEC++;
  for(byte i = 0; i <=15; i++) {
    resp[i] = --barsRA >= 0 ? 255 : ' ';
    resp[i + 16] = --barsDEC >= 0 ? 255 : ' ';
  }
  resp[32] = 0;
}
//-------------------------------------------------------------------------------------------------
void move2Eq() {              //  :MS#
  if (slewRAdir || slewDECdir) return;            // Slewing already
  if (ee.align == 'L') return;

  eq2altAz(objRA, objDEC);
  if (telALT < 0) {
    if (homeStat != '2') {
      sendMsg("1Object below    horizon.        "); // Object below horizon
      homeStat = '0';
    }
    return;
  }
  updateCoords();
  if (homeStat != '2') {
    targRA = objRA;
    targDEC = objDEC;
  }
  slewRAdir  = (targRA  > telRA)  - (targRA  < telRA);
  slewDECdir = (targDEC > telDEC) - (targDEC < telDEC);

  long distRA  = abs(targRA - telRA);
  long distDEC = abs(targDEC - telDEC);
  if (distRA > 43200) {
    distRA = 86200 - distRA;
    slewRAdir = -slewRAdir;
  }

  slewStartTime = millis();
  slewTimerRA  = slewStartTime + (distRA * 15 * 1000) / 3600 / slewRate;  // Get degs to slew, divide by degs/sec (360 / 24 = 15)
  slewTimerDEC = slewStartTime + (distDEC * 1000)     / 3600 / slewRate;  // 1 sec = 1000ms
  telRAb4  = telRA;
  telDECb4 = telDEC;
  slewAltAz = 0;
  resp[0] = '0';
}
//-------------------------------------------------------------------------------------------------
void move2AltAz() {           //  :MA#
  long objRAtemp  = objRA;
  long objDECtemp = objDEC;
  altAz2eq();
  move2Eq();
  objRA  = objRAtemp;
  objDEC = objDECtemp;
  slewAltAz = 1;
}
//-------------------------------------------------------------------------------------------------
void homeSave() {             //    :hS#
  homeStat = '1';
  updateCoords();
  ee.homeDEC = telDEC;
  ee.homeHA = telRA - sidTime();
  EEPROM.put(0, ee);
}
//-------------------------------------------------------------------------------------------------
void homeSet() {              //    :hF#
  homeStat = '2';
  targDEC = ee.homeDEC;
  targRA = sidTime() + ee.homeHA;
  move2Eq();
}
//-------------------------------------------------------------------------------------------------
void homePark() {             //    :hP#
  homeStat = '4';
  targDEC = ee.homeDEC;
  targRA = sidTime() + ee.homeHA;
  move2Eq();
}
#pragma endregion MOVEMENT
#pragma region SITE DETAILS
//=================================================================================================
// SITE DETAILS
//=================================================================================================
void setLat() {               //  :StsDD:MM#
  int hem = 0, deg, min;
  if (sscanf(buf, "St+%02d:%02d", &deg, &min)) hem = 1;
  if (sscanf(buf, "St-%02d:%02d", &deg, &min)) hem = -1;
  if (deg > 90 || min > 59 || hem == 0 || deg < 0 || min < 0) return;
  ee.latitude[ee.siteNo] = (deg * 60 + min) * hem;
  resp[0] = '1';
  EEPROM.put(0, ee);
}
//-------------------------------------------------------------------------------------------------
void getLat() {               //  :GgDDD*MM#
  int deg, min, lat = abs(ee.latitude[ee.siteNo]);
  deg = int(lat / 60);
  min = lat - deg * 60;
  sprintf(resp, "+%02d\xDF%02d", deg, min);
  resp[0] = ee.latitude[ee.siteNo] < 0 ? '-' : '+';
}
//-------------------------------------------------------------------------------------------------
void setLong() {              //  :SgDDD*MM#
  int deg, min;
  if (!sscanf(buf, "Sg%03d*%02d", &deg, &min)) if (!sscanf(buf, "Sg%03d\xDF%02d", &deg, &min)) return;
  if (deg > 359 || deg < 0 || min > 59 || min < 0) return;
  ee.longitude[ee.siteNo] = (deg * 60 + min);
  resp[0] = '1';
}
//-------------------------------------------------------------------------------------------------
void getLong() {              //  :GgDDD*MM#
  int deg, min, lng = ee.longitude[ee.siteNo];
  deg = int(lng / 60);
  min = lng - deg * 60;
  sprintf(resp, "%03d\xDF%02d", deg, min);
  EEPROM.put(0, ee);
}
//-------------------------------------------------------------------------------------------------
void getSiteName() {          //  :GM#  :GN#  :GO#  :GP#
  if (!strchr("MNOP", buf[1])) return;
  byte n = buf[1] - 'M';
  memmove(resp, ee.siteNames[n], 3);
  for (byte i = 3; i <= 12; i++) resp[i] = ' ';     //PUT          #
  resp[13] = 0;
  if (ee.siteNo == n) resp[10] = '\x02';
}
//-------------------------------------------------------------------------------------------------
void setSiteName() {          //  :SM...#  :SN...#  :SO...#  :SP...#
  if (!strchr("MNOP", buf[1])) return;
  if (buf[5]) return;
  byte n = buf[1] - 'M';
  if (isUpperCase(buf[2]) && isUpperCase(buf[3]) && isUpperCase(buf[4])) {
    ee.siteNames[n][0] = buf[2];
    ee.siteNames[n][1] = buf[3];
    ee.siteNames[n][2] = buf[4];
    resp[0] = '1';
  }
  EEPROM.put(0, ee);
}
//-------------------------------------------------------------------------------------------------
void setSite() {              //  :WN#
  if((buf[1] >= '1') & (buf[1] <= '4')) ee.siteNo = buf[1] - '1';
}
#pragma endregion SITE DETAILS
#pragma region MISCELLANEOUS
//=================================================================================================
// MISCELLANEOUS
//=================================================================================================
void setAlign() {             //  :Ax#
  if (buf[1] != 'A' && buf[1] != 'L' && buf[1] != 'P') return;
  ee.align = buf[1];
  resp[0] = 0;
  EEPROM.put(0, ee);
}
//-------------------------------------------------------------------------------------------------
void getAlignMenu() {         //  :Gx#
  switch (buf[1]) {
    case '0': strcpy(resp, "ALTAZ           "); break;
    case '1': strcpy(resp, "POLAR           "); break;
    case '2': strcpy(resp, "LAND            "); break;
    default: return;
  }
  if (ee.siteNo == buf[0] - '0') resp[10] = '\x02';
}
#pragma endregion MISCELLANEOUS
#pragma region LIBRARY
//=================================================================================================
// LIBRARY
//=================================================================================================
void setObjType() {           //  :SyGPDCO#
  if (strlen(buf) <= 8);
  for (byte i = 0; i <= 4; i++) {
    objTypes[i] = buf[i + 2];
    objTypes[i+1] = 0;
    if (!objTypes[i]) break;
  }
  resp[0] = '1';
}
//-------------------------------------------------------------------------------------------------
void setHigher() {            //  :ShDD#  0 to 90 degs
  int v;
  if (!sscanf(resp, "Sh%02d", &v)) return;
  if (v < 0 || v > 90) return;
  higher = v;
  resp[1] = '1';
}
//-------------------------------------------------------------------------------------------------
void setLower() {             //  :SoDD#  0 to 90 degs
  int v;
  if (!sscanf(resp, "Sh%02d", &v)) return;
  if (v > 90) return;
  lower = v;
  resp[1] = '1';
}
//-------------------------------------------------------------------------------------------------
void getBrighter() {          //  :Gb#  (default +20.0) - returns sMM.M#
  int v = abs(brighter);
  sprintf(resp, "+%02d.%01d", v / 10, v % 10);
  if (brighter < 0) resp[0] = '-';
}
//-------------------------------------------------------------------------------------------------
void setBrighter() {          //  :SbsMM.M#
  int a = 0, b = 0, v;
  if (sscanf(buf, "Sb%d.%d", &a, &b) < 2) {
    if (!sscanf(buf, "Sb%d", &a)) return;
  }
  v = abs(a) *10 + b;
  v = a < 0 ? -v : v;
  if (v < -55 || v > 200) return;
  resp[0] = '1';
  brighter = v;
}
//-------------------------------------------------------------------------------------------------
void getFainter() {           //  :Gf#  (default -05.5) - returns sMM.M#
  int v = abs(fainter);
  sprintf(resp, "+%02d.%01d", v / 10, v % 10);
  if (fainter < 0) resp[0] = '-';
}
//-------------------------------------------------------------------------------------------------
void setFainter() {           //  :SfsMM.M#
  int a = 0, b = 0, v;
  if (sscanf(buf, "Sb%d.%d", &a, &b) < 2) {
    if (!sscanf(buf, "Sb%d", &a)) return;
  }
  v = abs(a) *10 + b;
  v = a < 0 ? -v : v;
  if (v < -55 || v > 200) return;
  resp[0] = '1';
  fainter = v;
}
//-------------------------------------------------------------------------------------------------
void setLarger() {            //  :SlNNN#
  int v;
  if (!sscanf(buf, "Sl%d", &v)) return;
  if (v < 0 || v > 200) return;
  resp[0] = '1';
  larger = v;
}
//-------------------------------------------------------------------------------------------------
void setSmaller() {           //  :SsNNN#
  int v;
  if (!sscanf(buf, "Ss%d", &v)) return;
  if (v < 0 || v > 200) return;
  resp[0] = '1';
  smaller = v;
}
//-------------------------------------------------------------------------------------------------
void setField() {             //  :SFNNN#
  int v;
  if (!sscanf(buf, "SF%d", &v)) return;
  if (v > 200) return;
  resp[0] = '1';
  field = v;
}
//-------------------------------------------------------------------------------------------------
void setCatalogObj() {        //  :LCNNNN#
  resp[0] = 0;
  int v;
  if (!sscanf(buf, "LC%d", &v)) return;
  switch (catalog) {
    case 0:
      if (v > 7840) v = -1;   // NGC
      randomSeed(v + 1000L);
      cngc = v;
      break;
    case 1:
      if (v > 5386) v = -1;   // IC
      randomSeed(v + 10000L);
      ic = v;
      break;
    case 2:
      if (v > 12921) v = -1;  // UGC
      randomSeed(v + 100000L);
      ugc = v;
      break;
  }
  if (v) {
    objRA = random(86400L);
    objDEC = random(648000L) - 324000L;
    objMag = random(100) + 50;
    objSize = random(600) + 8;
    objQual = random(7);
    objClass = random(6);
  }
  object = 2;
}
//-------------------------------------------------------------------------------------------------
void setMess() {              //  :LMNNNN#
  resp[0] = 0;
  int v;
  if (!sscanf(buf, "LM%d", &v)) return;
  if (v > 110) v = -1;
  else {
    randomSeed(v);
    objRA = random(86400L);
    objDEC = random(468000L) - 144000L;
    objMag = random(86) + 16;
    objSize = random(600) + 8;
    objQual = random(7);
    objClass = random(6);
  }
  object = 1;
  messier = v;
}
//-------------------------------------------------------------------------------------------------
void setStar() {              //  :LSNNNN#
/*  Astronomical Algorithms V2 Page 93 (13.4)
  sin d  = sin B cos e + cos B sin e sin l
*/
  const byte plMag[] = {84, 58, 1, 86, 74, 112, 156, 178, 244};
  const byte plSize[] = {8, 10, 0, 14, 39, 15, 3, 2, 0};
  const long plPeriod[] = {88, 225, 27, 687, 4333, 10759, 30687, 60190, 90582};
  resp[0] = 0;
  int v;
  if (!sscanf(buf, "LS%d", &v)) return;
  if (!((v >= 1 && v <= 351) || (v >= 901 && v <= 909))) v = -1;
  if (v) {
    randomSeed(v + 1000000L);
    objRA = random(86400L);
    objDEC = random(648000L) - 324000L;
    objMag = random(74) - 14;
    if (v >= 901) {
      long days = (jdx10(yr, mth, day) - jdx10(1992, 1, 1)) / 10;
      float l = normF(360 * (float(days + v * 100) / plPeriod[v - 901]), 360);
      objRA = long(l * 240);                                  // 24*60*60/360
      objDEC = long(asinD(0.397777156 * cosD(l)) * 3600);     // sinD(23.4392911) - ecliptic obliquity
      objMag =  int(plMag[v-901]) - 100;
      objSize = plSize[v - 901];
    }
  }
  object = 0;
  star = v;
}
//-------------------------------------------------------------------------------------------------
void getInfo() {
  //    M1    VP GLOBMAG 2.5 SZ  0.0'#
  int v;
  resp[0] = 0;
  const char type[6][5] = {"GLOB", "PNEB", "DNEB", "GAL ", "OPNB", "OPEN"};
  strcpy(resp," NGC0           MAG 0.0 SZ      ");
  switch (object) {
    case 0:
      v = star;
      memmove(resp, "STAR", 4);
      memmove(resp + 12, resp, 4);
      if (!starList && v >= 901) {
        strcpy_P(buf, (char *)pgm_read_word(&(messages[v-901])));
        memmove(resp, buf, 8);
        if (v == 903) memmove(resp + 16, "Phase: 23.5%\0", 12); // Moon phase
        else {
          sprintf(buf, "%d\"", objSize);
          memmove(resp + 32 - strlen(buf), buf, strlen(buf));    // Size
        }
        v = -2;
      } else resp[24] = resp[25] = ' ';
      if (starList) v = -1;
      break;
    case 1: memmove(resp, "   M", 4); v = messier;  break;
    case 2:
      switch (catalog) {
        case 0: memmove(resp, " NGC", 4); v = cngc; break;
        case 1: memmove(resp, "  IC", 4); v = ic;   break;
        case 2: memmove(resp, " UGC", 4); v = ugc;  break;
      }
      break;
  }
  if (object) {
    if (v > 0) {
      memmove(resp + 9, quality[objQual], 2);               // Quality
      memmove(resp + 12, type[objClass], 4);                // Class of object
      sprintf(buf, "%d.%d'", objSize / 10, objSize % 10);
      memmove(resp + 32 - strlen(buf), buf, strlen(buf));   // Size
    } else memmove(resp + 16, "Coordinates Only", 16);
  }
  if (v > 0) {
    sprintf(buf, "%d", v);
    memmove(resp + 4, buf, strlen(buf));                      // Object no.
    sprintf(buf, "%d.%d", abs(objMag) / 10, abs(objMag) % 10);
    if (objMag < 0) resp[19] = '-';
    memmove(resp + 23 - strlen(buf), buf, strlen(buf));       // Magnitude
  }
  if (v == -1) strcpy(resp, "No matching     object...       "); 
}
//-------------------------------------------------------------------------------------------------
void setCatalog() {           //  :LoN# - 0, 1 or 2
  int v;
  if (!sscanf(buf, "Lo%d", &v)) return;
  if (v < 0 || v > 2) return;
  resp[0] = '1';
  catalog = v;
}
//-------------------------------------------------------------------------------------------------
void setStarType() {          //  :LsN# - 0, 1 or 2
  int v;
  if (!sscanf(buf, "Lo%d", &v)) return;
  if (v < 0 || v > 2) return;
  resp[0] = '1';
  starList = v;
}
#pragma endregion LIBRARY
#pragma region ENGINE
//=================================================================================================
// ENGINE
//=================================================================================================
void updateCoords() {
  unsigned long millisNow = millis();
  long mvt;

  // DEAL WITH SLEWING
  if (slewRAdir) {                                // Deal with telescope RA slew
    moveRAmillis = millisNow;
    if(millisNow > slewTimerRA) {                 // Finished slewing in RA?
      if (slewAltAz) altAz2eq();
      telRA = targRA;
      slewRAdir = 0;
    }
    else {
      mvt = (240L * slewRAdir * (millisNow - slewStartTime) * slewRate); // (60*60/15 = 240)
      mvt /= 1000;
      telRA = telRAb4 + mvt;
    }
  }
  if (slewDECdir) {                               // Deal with telescope DEC slew
    if(millisNow > slewTimerDEC) {                // Finished slewing in DEC?
      if (slewAltAz) altAz2eq();
      telDEC  = targDEC;
      slewDECdir = 0;
    }
    else {
      mvt = (3600L * slewDECdir * (millisNow - slewStartTime) * slewRate * 15);  // (60*60 = 3600)
      mvt /= 1000;
      telDEC = telDECb4 + mvt;
    }
  }

  // DEAL WITH PULSE MOVES, TRACKING & DRIVE FREQUENCY
  if (!(slewRAdir || slewDECdir)) {
    if (homeStat == '2' || homeStat == '4') {       // Telescope is parked - update coords
      telRA == sidTime() + ee.homeHA;
      homeStat == '1';                              // Home position found
    }
    if (moveRAdir) {                                // RA pulse move?
      mvt = (millisNow - moveRAmillis) * moveRAdir * (moveRate - 1);    // moveRate: G = 2, C = 32, F = 480, S = 240 * slewRate
      telRAms += mvt;
      telRAms %= 1000;
      mvt += telRAms;
      mvt /= 1000;
      telRA += mvt;                                 // Pulse move
      homeStat = '0';
    }
    else if (freq != 601 && !freqQ) telRA += long(float(millisNow - moveRAmillis) * 601.0 / float(freq)); // Account for drive freq

    if (moveDECdir) {                               // DEC pulse move?
      mvt = (millisNow - moveDECmillis) * moveDECdir * moveRate * 15;   // moveRate: G = 30, C = 480, F = 7200, S = 3600 * slewRate
      mvt /= 1000;
      telDEC += mvt;                                 // Pulse move
      homeStat = '0';
    }
  }
  moveRAmillis = millisNow;
  moveDECmillis = millisNow;    

  // NORMALISE TELESCOPE COORDINATES AND DIRECTIONS
  if (telDEC > 324000 || telDEC < -324000) {            // Has telescope passed over the NCP or SCP?
    telDEC = (telDEC > 0 ? 648000 : -648000) - telDEC;  // Fix DEC
    telRA = telRA + 43200;                              // On otherside of celestial sphere
    slewDECdir = -slewDECdir;                           // Flip DEC directions
    moveDECdir = -moveDECdir;
  }
  telRA = normL(telRA, 86400);                          // (24*60*60)
}
//-------------------------------------------------------------------------------------------------
void eq2altAz(long ra, long dec) {
/*  Astronomical Algorithms V2 page 93 (13.5 & 13.6)
    tan A = (sin H)/(cos H sin l - tan d cos l)
    sin h = sin l sin d + cos l cos d cos H     */
  float sid = sidTime() / 240.0;                            // 24*60*60/(15*24)=240
  float L = float(ee.longitude[ee.siteNo]) / 60.0;  	      // Longitude
  float d = float(dec) / 3600.0;              		          // Declination (360*60*60/360 = 3600)
  float H = normF(sid - float(ra) / 240.0 - L, 360); 			  // Hour Angle (inc W from S) 24*60*60/(15*24)=240
  float l = float(ee.latitude[ee.siteNo]) / 60.0;		        // Latitude
  float A = atan2D(sinD(H), cosD(H) * sinD(l) - tanD(d) * cosD(l)) * 3600.0;  // (360*60*60)/360=3600
  float h = asinD(sinD(l) * sinD(d) + cosD(l) * cosD(d) * cosD(H)) * 3600.0;
  telAZ  = A;                                  
  telALT = h;
}
//-------------------------------------------------------------------------------------------------
void altAz2eq() {
/*  Astronomical Algorithms V2 page 94 (13.6b)
    tan H = (sin A) / (cos A sin l + tan h cos l)
    sin d = sin l sin h - cos l cos h cos A       */
  float A = telAZ  / 3600;                                              // Azimuth
  float h = telALT / 3600;                                              // Altitude
  float phi = ee.latitude[ee.siteNo]/ 60;                               // Latitude

  float H = atan2D(sinD(A), cosD(A) * sinD(phi) + tanD(h) * cosD(phi)); // Hour Angle (inc W from S)
  float d = asinD(sinD(phi) * sinD(h) - cosD(phi) * cosD(h) * cosD(A));
  objDEC = long(float(d * 3600.0));
  objRA =  normL(long(normF(H, 360) * 240.0 + sidTime()), 86400);
}
//-------------------------------------------------------------------------------------------------
void updateTime() {
  unsigned long millisNow = millis();
  unsigned long elapsed = (millisNow - clock);
  clock = millisNow;

  ms += elapsed % 60000;
  if (ms >= 60000) {
    min++;
    ms %= 60000;
  }
  elapsed /= 60000;
  min += elapsed % 60;
  if (min > 59) {
    hr++;
    min %= 60;
  }
  elapsed /= 60;
  hr += elapsed % 24;
  if (hr > 23){
    day++;
    hr %=24;
  }
  elapsed /= 24;
  switch (mth) {
    case 2: if (day > (28 + (yr % 4 == 0))){
              mth++;
              day %= (28 + (yr % 4 == 0));
              elapsed /= (28 + (yr % 4 == 0));
            }
            break;
    case 4: if (day > 30) {
              mth++;
              day %= 30;
              elapsed /= 30;
            }
            break;
    case 6: if (day > 30) {
             mth++;
              day %= 30;
              elapsed /= 30;
            }
            break;
    case 9: if (day > 30) {
              mth++;
              day %= 30;
              elapsed /= 30;
            }
            break;
    case 11: if (day > 30) {
              mth++;
              day %= 30;
              elapsed /= 30;
            }
            break;
    defualt: if (day > 31) {
              mth++;
              day %= 31;
              elapsed /= 31;
            }
            break;
  }
  if (mth > 12) {
    mth = 1;
    yr += elapsed;
  }
  return;
}
//-------------------------------------------------------------------------------------------------
long jdx10(int y, byte m, byte d) {
  int a, b;
  long i, j;

  if(m < 3) {
   y--;
    m+= 12;
  }
  a = int(y / 100);
  b = 2 - a + int(a / 4);
  i = (float)(365.25 * (y + 4716));
  j = 30.6001 * (m + 1);
  return (i + j + d + b)*10 - 15245L;
}
//-------------------------------------------------------------------------------------------------
unsigned long sidTime() {    // Returns value >= 0 and < 86400 (24 * 60 * 60 * 1000)
  updateTime();
  long t = (jdx10(yr, mth ,day) - 24515450);
  float T = (float)t / 365250.0;
  float th0 = normF((100.46061837 + 36000.770053608 * T + 0.000387933 * T * T - T * T * T / 37810000), 360); // In degrees
        th0 += float(hr + min / 60.0 + ms / 3600000.0) * 15 * sol2sid;
        th0 += -(ee.longitude[ee.siteNo] / 60.0);
        th0 += sidOffset - ee.hourOffset * 15;          // (360/24=15)
        th0 = normF(th0, 360);
  return long(th0 * 240.0);                             // (24*60*60/360=240)
}
//-------------------------------------------------------------------------------------------------
float sinD(float v) {
  return sin(v * deg2rad);
}
//-------------------------------------------------------------------------------------------------
float cosD(float v) {
  return cos(v * deg2rad);
}
//-------------------------------------------------------------------------------------------------
float tanD(float v) {
  return tan(v * deg2rad);
}
//-------------------------------------------------------------------------------------------------
float asinD(float v) {
  return asin(v) * rad2deg;
}
//-------------------------------------------------------------------------------------------------
float atan2D(float a, float b) {
  return normF(atan2(a, b) * rad2deg, 360);
}
//-------------------------------------------------------------------------------------------------
long normL(long v, long r) {
  return v % r + (v < 0) * r;
}
//-------------------------------------------------------------------------------------------------
float normF(float v, float r) {
  return v - int (v / r) * r + (v < 0) * r;
}
#pragma endregion ENGINE
#pragma region JUNKYARD
#pragma endregion JUNKYARD
// END