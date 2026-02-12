/*  Programmable Multi Effects Expression Pedal
 *  
 *  Main hardware:
 *  - RP2040 microcontroller board.
 *  - PT8211 DAC
 *
 *  Software: 
 *  - Earle Phil Hower Pi Pico Core
 *  - default Arduino Libraries
 *
 *  Features:
 *  - 2 analog inputs (audio, footwitch)
 *  - 2 digital encoders
 *  - 4 digital inputs (switches)
 *  - 4 DIP switches
 *  - FULL stereo signal
 *
 *  by Barito 2025 (last update jan 2025)
 *  
 */

#include <I2S.h>
#include <ADCInput.h>
#include <EEPROM.h>

#define MAX_CHAINS 16
#define MAX_FX_IN_CHAIN 4

#define SAMPLE_RATE 40000 //MAX sample rate

#define EEPROM_BASE 0
#define FXPAR_SIZE (MAX_CHAINS * MAX_FX_IN_CHAIN * 3)
#define EEPROM_MAGIC 0xA5
#define EEPROM_MAGIC_ADDR 510
volatile bool requestSave = false;

#define DEBUG

//ANALOG PINS
const int16_t AUDIO_PIN = 26;
const int16_t FOOT_POT_PIN = 27;
//const int16_t POT1_PIN 28 //A2 (used by encoder 1)
//const int16_t POT2_PIN 29 //A3 (used by encoder 2)
int16_t footMax = 0; //max range init
int16_t footMin = 4095; //min range init
uint16_t pot_timer;
int32_t deb_timer;
bool footPickupActive = false;

//DIP SWITCHES
const int16_t DIP_PIN[4] = {12, 13, 14, 15};
uint16_t dip_timer;

//I2S COMMUNICATION
const int16_t I2S_BCLK = 2;
//const int16_t I2S_WS = 3; // Forced to BCLK+1
const int16_t I2S_DOUT = 4;

//SWITCHES
const int16_t SW_PIN[4] = {5, 28, 29, 7}; //Built-in switch (SW1), encoders (E1 and E2), foot switch
bool SW_state[4];
bool BYPASS;
uint16_t sw_timer;

//LED
const int16_t LED_pin = 6;

//ROTARY (AUX BOARD)
const int16_t ROT_E1_PIN[2] = {8, 9}; //E1_A, E1_B
const int16_t ROT_E2_PIN[2] = {10, 11}; //E2_A, E2_B
bool enc_dummy[2];
uint16_t enc_timer;

//FX VARIABLES
int16_t FX_num;
uint8_t activeChain = 0; //active chain
int8_t activeFX = 0; // numero FX reale

//delay
#define DELAY_SAMPLES 16000
int16_t delayBufferL[DELAY_SAMPLES] = {0};
int16_t delayBufferR[DELAY_SAMPLES] = {0};
uint32_t delayIndex = 0;
uint16_t DUMMY_timer;

//octaver
#define OCT_SAMPLES 2048
int16_t octBuffer[OCT_SAMPLES] = {0};
uint32_t octWriteIndex = 0;
float octReadIndex = 0.0f;

//reverb
#define RVB_SAMPLES 6000
int16_t rvbBufL[RVB_SAMPLES];
int16_t rvbBufR[RVB_SAMPLES];
uint32_t rvbIdx = 0;

//ADC raw values
uint16_t audioValue_raw, footValue_raw/*, pot1Value_raw, pot2Value_raw*/;

//ADC "debounced" values
uint16_t footValue_q/*, pot1Value_deb, pot2Value_deb*/;

//processed audio
int16_t procAudio_left, procAudio_right;

//DAC audio
int16_t dacAudio_left, dacAudio_right;

I2S i2s(OUTPUT);
ADCInput adc(AUDIO_PIN, FOOT_POT_PIN/*, POT1_PIN, POT2_PIN*/); //audio signal, expression pedal, AUX pot 1, AUX pot 2)

uint8_t fxChains[MAX_CHAINS][MAX_FX_IN_CHAIN] = {
  {0, 2, 6},   // catena 0 (0.0.0.0)
  {0, 2, 7},   // catena 1 (1.0.0.0)
  {0, 2, 8},   // catena 2 (0.1.0.0)
  {0, 3, 5},   // catena 3 (1.1.0.0)
  {0, 3, 6},   // catena 4 (0.0.1.0)
  {0, 3, 7},   // catena 5 (1.0.1.0)
  {0, 3, 8},   // catena 6 (0.1.1.0)
  {1, 2, 5},   // catena 7 (1.1.1.0)
  {1, 2, 7},   // catena 8 (0.0.0.1)
  {1, 3, 5},   // catena 9 (1.0.0.1)
  {1, 4, 5},   // catena 10 (0.1.0.1)
  {2},   // catena 11 (1.1.0.1)
  {5},   // catena 12 (0.0.1.1)
  {6},   // catena 13 (1.0.1.1)
  {7},   // catena 14 (0.1.1.1)
  {8}    // catena 15 (1.1.1.1)
};

uint8_t chainLength[MAX_CHAINS] = {
  3,  // lunghezza catena 0
  3,  // lunghezza catena 1
  3,  // lunghezza catena 2
  3,  // lunghezza catena 3
  3,  // lunghezza catena 4
  3,  // lunghezza catena 5
  3,  // lunghezza catena 6
  3,  // lunghezza catena 7
  3,  // lunghezza catena 8
  3,  // lunghezza catena 9
  3,  // lunghezza catena 10
  1,  // lunghezza catena 11
  1,  // lunghezza catena 12
  1,  // lunghezza catena 13
  1,  // lunghezza catena 14
  1   // lunghezza catena 15
};

struct Encoder {
  uint8_t last;
  int16_t value;
};
Encoder enc[2];   // enc[0] = Encoder 1, enc[1] = Encoder 2

uint8_t FXParVal[MAX_CHAINS][MAX_FX_IN_CHAIN][3] = {0}; //0 - EXPpedal, 1 - encoder 1, 2 - encoder 2

void setup() {
  //built-in switch, encoders switches
  for (int a=0; a<3; a++){
    pinMode(SW_PIN[a], INPUT_PULLUP);
    SW_state[a] = digitalRead(SW_PIN[a]);
  }
  //foot switch monitor
  pinMode(SW_PIN[3], INPUT); //foot switch state monitor (pulled up in hardware)
  SW_state[3] = digitalRead(SW_PIN[3]);
  //dip switch
  for (int b=0; b<4; b++){
    pinMode(DIP_PIN[b], INPUT_PULLUP);
    //DIP_state[b] = digitalRead(DIP_PIN[b]);
  }
  //LED 
  pinMode(LED_pin, OUTPUT);
  digitalWrite(LED_pin, LOW);
  //rotary
  pinMode(ROT_E1_PIN[0], INPUT_PULLUP);
  pinMode(ROT_E1_PIN[1], INPUT_PULLUP);
  pinMode(ROT_E2_PIN[0], INPUT_PULLUP);
  pinMode(ROT_E2_PIN[1], INPUT_PULLUP);

  // encoders, stato iniziale Gray-code
  enc[0].last = (digitalRead(ROT_E1_PIN[0]) << 1) | digitalRead(ROT_E1_PIN[1]);
  enc[1].last = (digitalRead(ROT_E2_PIN[0]) << 1) | digitalRead(ROT_E2_PIN[1]);
  enc[0].value = 0;
  enc[1].value = 0;
  //I2S, ADC, EEPROM, SERIAL
  i2s.setBCLK(I2S_BCLK); // Note: LRCLK (WS)= BCLK + 1
  i2s.setDOUT(I2S_DOUT);
  i2s.setBitsPerSample(16);
  i2s.setLSBJFormat();
  i2s.setBuffers(8, 32); //DMA buffers number and dimension
  adc.setBuffers(8, 32);
  i2s.begin(SAMPLE_RATE);
  adc.begin(SAMPLE_RATE);
  #ifdef DEBUG
    Serial.begin(115200);
  #endif
  EEPROM.begin(512);    // spazio riservato in Flash (FXParVal 16 × 4 × 3 = 192 byte)
  LoadFXParVal();       // carica i parametri salvati in Flash
  Chain_Read();         // legge la catena in uso
  LoadFX();             // carica i parametri del primo FX della catena
}

void loop() {
  while (adc.available()) {
    audioValue_raw = adc.read(); //Read (and store in DMA) audio sample (12 bit -> 0–4095)
    footValue_raw = adc.read(); //Foot switch (12 bit -> 0–4095)
    //pot1Value_raw = adc.read(); //potentiometer #1 (12 bit -> 0–4095)
    //pot2Value_raw = adc.read(); //potentiometer #2 (12 bit -> 0–4095)
    Process_AUDIO(); //Here is where the magic happens
    i2s.write16(dacAudio_left, dacAudio_right); // send to DAC, LEFT and RIGHT channel (16-bit, signed)
  }
  SW_Read();
  Pot_Read();
  Encoder_Read();
  MEM_Save();
  #ifdef DEBUG
    Debug();
  #endif
}

void MEM_Save(){
  if (requestSave) {
    requestSave = false;
    // ---- PAUSA AUDIO ----
    i2s.end();
    adc.end();
    delay(5);   // lascia svuotare FIFO
    SaveFXParVal();
    delay(5);
    #ifdef DEBUG
      Serial.println("FX parameters SAVED");
      delay(1000);
    #endif
    // ---- RIPARTE AUDIO ----
    i2s.begin(SAMPLE_RATE);
    adc.begin(SAMPLE_RATE);
  }
}

void LoadFX(){
  footValue_q = FXParVal[activeChain][activeFX][0];
  enc[0].value = FXParVal[activeChain][activeFX][1];
  enc[1].value = FXParVal[activeChain][activeFX][2];
}

void SaveFXParVal() {
  int addr = EEPROM_BASE;
  for (uint8_t c = 0; c < MAX_CHAINS; c++) {
    for (uint8_t f = 0; f < MAX_FX_IN_CHAIN; f++) {
      for (uint8_t p = 0; p < 3; p++) {
        EEPROM.write(addr++, FXParVal[c][f][p]);
      }
    }
  }
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  EEPROM.commit();
}

void LoadFXParVal() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    memset(FXParVal, 0, sizeof(FXParVal));
    return;
  }
  int addr = EEPROM_BASE;
  for (uint8_t c = 0; c < MAX_CHAINS; c++) {
    for (uint8_t f = 0; f < MAX_FX_IN_CHAIN; f++) {
      for (uint8_t p = 0; p < 3; p++) {
        FXParVal[c][f][p] = EEPROM.read(addr++);
      }
    }
  }
}

void Process_AUDIO(){
  procAudio_left = (int16_t)audioValue_raw - 2048; //12 bit unsigned -> 12 bit signed
  procAudio_right = procAudio_left;
  int len = chainLength[activeChain];
  for (uint8_t slot = 0; slot < len; slot++) {
    switch (fxChains[activeChain][slot]) {
      case 0: COMPRESSOR(slot); break;
      case 1: OCTAVER(slot); break;
      case 2: DISTORTION(slot); break;
      case 3: DAFT_DISTORTION(slot); break;
      case 4: BIT_CRUSH(slot); break;
      case 5: DELAY(slot); break;
      case 6: CHORUS(slot); break;
      case 7: FLANGER(slot); break;
      case 8: REVERB(slot); break;
    }
  }

  procAudio_left  = clamp(procAudio_left, -2048, 2047);
  procAudio_right = clamp(procAudio_right, -2048, 2047);

  dacAudio_left  = procAudio_left << 4; //12 bit signed -> 16 bit signed
  if(!BYPASS){ //only left channel is true bypass
    dacAudio_right = procAudio_right << 4;
  }
  else{
    dacAudio_right = 0;
  }
}

// ==================== COMMON UTILS ====================

inline int16_t deadZone(int16_t x,int16_t t=6){ 
  return (abs(x)<t)?0:x; 
}

inline int16_t smoothLowLevel(int16_t x){ 
  static int16_t last=0; 
  if(abs(x)<32)x=(x+last)>>1; 
  last=x; 
  return x; 
}

inline int16_t tubeClipExtreme(int32_t x, int32_t headroom) {
  if (x > headroom)
      return headroom + ((x - headroom) >> 2);     // positivo morbido
  if (x < -headroom + (headroom >> 3))
      return -headroom + ((x + headroom) >> 3);    // negativo più duro
  return x;
}

inline int16_t clamp(int32_t x, int16_t lo, int16_t hi) { 
  if (x > hi)  return hi; 
  if (x < lo)  return lo; 
  return x;
}

inline void readEncoder(uint8_t idx, const int8_t pinA, const int8_t pinB, uint8_t e_step){
  uint8_t a = digitalRead(pinA);
  uint8_t b = digitalRead(pinB);
  uint8_t encoded = (a << 1) | b;
  uint8_t sum = (enc[idx].last << 2) | encoded;
  if (sum == 0b1000) {          // CW
    if (enc[idx].value < 127 - e_step)
      enc[idx].value +=5;
  }
  else if (sum == 0b0010) {     // CCW
    if (enc[idx].value > 0 + e_step)
      enc[idx].value -=5;
  }
  enc[idx].last = encoded;
}

void COMPRESSOR(uint8_t slot){

  // ===== INPUT =====
  int32_t inL = deadZone(procAudio_left);
  int32_t inR = deadZone(procAudio_right);

  // ===== PARAMETRI =====
  int32_t sustain = FXParVal[activeChain][slot][0];   // 0..127
  int32_t attackP = FXParVal[activeChain][slot][1];   // 0..127
  int32_t level   = FXParVal[activeChain][slot][2];   // 0..127

  // ===== THRESHOLD (più basso!) =====
  int32_t threshold = 32 + ((127 - sustain) * 12);    // ~32..1556

  // ===== ATTACK / RELEASE =====
  int32_t atk = 1 + (attackP >> 4);   // 1..8  (lento = più punch)
  int32_t rel = 32 + (sustain >> 1);  // lungo = sustain

  // ===== ENVELOPE =====
  static int32_t envL = 0;
  static int32_t envR = 0;

  int32_t absL = abs(inL);
  int32_t absR = abs(inR);

  envL += (absL - envL) >> ((absL > envL) ? atk : rel);
  envR += (absR - envR) >> ((absR > envR) ? atk : rel);

  // ===== GAIN REDUCTION (hard knee) =====
  int32_t gainL = 2048;
  int32_t gainR = 2048;

  if(envL > threshold){
    int32_t over = envL - threshold;
    gainL = 2048 - (over >> 1);   // molto più deciso
  }

  if(envR > threshold){
    int32_t over = envR - threshold;
    gainR = 2048 - (over >> 1);
  }

  // clamp forte → vero sustain
  if(gainL < 256) gainL = 256;
  if(gainR < 256) gainR = 256;

  // ===== APPLY GAIN =====
  int32_t wetL = (inL * gainL) >> 11;
  int32_t wetR = (inR * gainR) >> 11;

  // ===== MAKE-UP =====
  int32_t makeup = 1024 + level * 32;
  wetL = (wetL * makeup) >> 10;
  wetR = (wetR * makeup) >> 10;

  // ===== OUTPUT =====
  procAudio_left  = clamp(wetL, -2048, 2047);
  procAudio_right = clamp((inR >> 1) + (wetR >> 1), -2048, 2047);
}

void OCTAVER(uint8_t slot) {

  // ===== INPUT =====
  int32_t inL = deadZone(procAudio_left);
  int32_t inR = deadZone(procAudio_right);

  // ===== OTTAVA CANALE SINISTRO =====
  static int8_t octShiftL = -2;
  static int8_t lastEncL  = 0;
  int8_t deltaL = FXParVal[activeChain][slot][1] - lastEncL;
  if(deltaL != 0){
      octShiftL += (deltaL > 0) ? 1 : -1;
      if(octShiftL >  1) octShiftL = -2;
      if(octShiftL < -2) octShiftL =  1;
      lastEncL = FXParVal[activeChain][slot][1];
  }

  uint16_t readIncL = 256; // base increment
  switch(octShiftL){
      case -2: readIncL = 64;   break;  // 0.25x
      case -1: readIncL = 128;  break;  // 0.5x
      case  0: readIncL = 256;  break;  // 1x
      case  1: readIncL = 512;  break;  // 2x
  }

  // ===== OTTAVA CANALE DESTRO =====
  static int8_t octShiftR = -2;
  static int8_t lastEncR  = 0;
  int8_t deltaR = FXParVal[activeChain][slot][2] - lastEncR;
  if(deltaR != 0){
      octShiftR += (deltaR > 0) ? 1 : -1;
      if(octShiftR >  1) octShiftR = -2;
      if(octShiftR < -2) octShiftR =  1;
      lastEncR = FXParVal[activeChain][slot][2];
  }

  uint16_t readIncR = 256;
  switch(octShiftR){
      case -2: readIncR = 64;   break;
      case -1: readIncR = 128;  break;
      case  0: readIncR = 256;  break;
      case  1: readIncR = 512;  break;
  }

  // ===== Aggiorna indici di lettura =====
  static uint32_t readIndexL = 0;
  static uint32_t readIndexR = 0;

  readIndexL += readIncL;
  readIndexR += readIncR;

  while(readIndexL >= OCT_SAMPLES << 8) readIndexL -= OCT_SAMPLES << 8;
  while(readIndexR >= OCT_SAMPLES << 8) readIndexR -= OCT_SAMPLES << 8;

  // ===== Interpolazione lineare semplice =====
  uint32_t i0L = readIndexL >> 8;
  uint32_t i1L = (i0L + 1) % OCT_SAMPLES;
  uint16_t fracL = readIndexL & 0xFF;
  int32_t wetL = ((octBuffer[i0L] * (256 - fracL)) + (octBuffer[i1L] * fracL)) >> 8;

  uint32_t i0R = readIndexR >> 8;
  uint32_t i1R = (i0R + 1) % OCT_SAMPLES;
  uint16_t fracR = readIndexR & 0xFF;
  int32_t wetR = ((octBuffer[i0R] * (256 - fracR)) + (octBuffer[i1R] * fracR)) >> 8;

  // ===== Smoothing leggero manuale (filtro esponenziale integer) =====
  static int32_t smoothWetL = 0;
  static int32_t smoothWetR = 0;

  smoothWetL = (smoothWetL * 15 + wetL) >> 4; // ~ alpha = 1/16
  smoothWetR = (smoothWetR * 15 + wetR) >> 4;

  // ===== Scrittura buffer circolare =====
  octBuffer[octWriteIndex] = inL;
  octWriteIndex = (octWriteIndex + 1) % OCT_SAMPLES;

  // ===== MIX DRY/WET =====
  procAudio_left = (int16_t)smoothWetL;                  // solo wet sinistro
  procAudio_right = (int16_t)((inR >> 1) + (smoothWetR >> 1)); // dry/wet 50% destro
}

void DISTORTION(uint8_t slot) {

  // ===== INPUT =====
  int32_t inL = deadZone(procAudio_left);
  int32_t inR = deadZone(procAudio_right);

  // ===== PARAMETRI =====
  int32_t drive = FXParVal[activeChain][slot][0];   // 0..127
  int32_t body  = FXParVal[activeChain][slot][1];   // volume / corpo
  int32_t tone  = FXParVal[activeChain][slot][2];   // tono

  // ===== PRE GAIN (NON LINEARE) =====
  int32_t preGain = 256 + drive * drive;            // cresce esponenziale
  int32_t xL = (inL * preGain) >> 8;
  int32_t xR = (inR * preGain) >> 8;

  // ===== SOFT SATURATION (RATIONAL SHAPER) =====
  // y = x / (1 + |x|)
  auto shape = [](int32_t x){
    int32_t ax = abs(x);
    return (x << 11) / (ax + 2048);
  };

  xL = shape(xL);
  xR = shape(xR);

  // ===== ASIMMETRIA "VALVOLA" =====
  if (xL < 0) xL = (xL * 3) >> 2;
  if (xR < 0) xR = (xR * 3) >> 2;

  // ===== POST LOWPASS (ANTI FIZZ) =====
  // tone: 0 scuro — 127 aperto
  float lp = 0.02f + (tone / 127.0f) * 0.18f;

  static int32_t lpL = 0;
  static int32_t lpR = 0;
  lpL += (xL - lpL) * lp;
  lpR += (xR - lpR) * lp;

  // ===== OUTPUT GAIN =====
  int32_t outGain = 128 + body * 32;
  int32_t wetL = (lpL * outGain) >> 11;
  int32_t wetR = (lpR * outGain) >> 11;

  // ===== OUTPUT =====
  procAudio_left  = clamp(wetL, -2048, 2047);
  procAudio_right = clamp((inR >> 1) + (wetR >> 1), -2048, 2047);
}

void BIT_CRUSH(uint8_t slot){

  // ===== INPUT =====
  int16_t inL = deadZone(procAudio_left);
  int16_t inR = deadZone(procAudio_right);

  float drive = (float)FXParVal[activeChain][slot][0] / 127.0f;
  drive = 1.0f + drive * 5.0f;

  int32_t drivenL = clamp(inL * drive, -2048, 2047);
  int32_t drivenR = clamp(inR * drive, -2048, 2047);

  uint8_t bits = 12 - (FXParVal[activeChain][slot][1] >> 4);
  if(bits < 4) bits = 4;
  uint16_t mask = ~((1 << (12 - bits)) - 1);

  int16_t crushedL = (drivenL >> 2) & mask;
  int16_t crushedR = (drivenR >> 2) & mask;

  uint16_t decim = 1 + (FXParVal[activeChain][slot][2] >> 4);
  static uint16_t cnt = 0;
  static int16_t heldL = 0;
  static int16_t heldR = 0;

  if(++cnt >= decim){
      cnt = 0;
      heldL = crushedL;
      heldR = crushedR;
  }

  procAudio_left = (int16_t)heldL;
  procAudio_right = (int16_t)constrain(inR * 0.5f + heldR * 0.5f, -2048.0f, 2047.0f);
}

void DAFT_DISTORTION(uint8_t slot) {
  
  // ===== INPUT =====
  int32_t inL = deadZone(procAudio_left);
  int32_t inR = deadZone(procAudio_right);

  // ===== DRIVE (EXP PEDAL) =====
  float drive = 1.0f + 8.0f * ((float)FXParVal[activeChain][slot][0] / 127.0f);

  // ===== CLAMP =====
  int32_t xL = clamp(inL * drive, -2048, 2047);
  int32_t xR = clamp(inR * drive, -2048, 2047);

  // ===== TONE FILTER =====
  //float dftoneCoef = 0.05f + (1.0f - (float)FXParVal[activeChain][slot][1] / 127.0f) * 0.45f;

  float dftoneCoef = 0.10f;   // massimo 0.05
  static int32_t dftoneL = 0;
  static int32_t dftoneR = 0;

  dftoneL += (xL - dftoneL) * dftoneCoef;
  dftoneR += (xR - dftoneR) * dftoneCoef;

  // ===== DAFT SAMPLE RATE REDUCTION (EXP PEDAL) =====
  static uint16_t daftCount = 0;
  daftCount++;
  uint16_t step = FXParVal[activeChain][slot][0] >> 2;
  if(step == 0) step = 1;

  static int32_t heldL = 0;
  static int32_t heldR = 0;

  if(daftCount >= step){
      daftCount = 0;
      heldL = dftoneL;
      heldR = dftoneR;
  }

  // ===== STEREO WIDTH (ENCODER 1) =====
  // encoder1: 0..127 → max ±4 samples offset
  int32_t widthOffset = (FXParVal[activeChain][slot][2] - 64) / 16; // -4 .. +3
  static int32_t bufR[8] = {0};
  static uint8_t idxR = 0;
  bufR[idxR] = heldR;
  uint8_t readIdx = (idxR + 8 + widthOffset) % 8;
  int32_t wetR = bufR[readIdx];
  idxR = (idxR + 1) % 8;

  // ===== OUTPUT TRUE STEREO =====
  procAudio_left = (int16_t)heldL;               // SINISTRO: solo wet
  procAudio_right = (int16_t)((inR >> 1) + (wetR >> 1)); // DESTRO: 50% dry + 50% wet
}

void DELAY(uint8_t slot){
  
  // ===== INPUT =====
  int32_t inL = deadZone(procAudio_left);
  int32_t inR = deadZone(procAudio_right);

  // ===== TEMPO BASE =====
  uint32_t delayBase = 400 + ((uint32_t)FXParVal[activeChain][slot][1] * 15600) / 127;

  // ===== DELTA STEREO =====
  int32_t delta = map(FXParVal[activeChain][slot][2], 0, 127, -800, +800);

  int32_t delayL = delayBase - delta;
  int32_t delayR = delayBase + delta;

  delayL = constrain(delayL, 1, DELAY_SAMPLES-1);
  delayR = constrain(delayR, 1, DELAY_SAMPLES-1);

  // ===== FEEDBACK =====
  float fb = (float)FXParVal[activeChain][slot][0] / 127.0f;
  fb = fb * fb;
  if(fb > 0.97f) fb = 0.97f;

  // ===== READ =====
  uint32_t readL = (delayIndex + DELAY_SAMPLES - delayL) % DELAY_SAMPLES;
  uint32_t readR = (delayIndex + DELAY_SAMPLES - delayR) % DELAY_SAMPLES;

  int16_t wetL = delayBufferL[readL];
  int16_t wetR = delayBufferR[readR];

  // ===== WRITE =====
  int16_t writeL = inL + wetL * fb;
  int16_t writeR = inR + wetR * fb;

  float dltoneCoef = 0.10f;   // FILTRO PB (max 0.05)
  static int32_t dltoneL = 0;
  static int32_t dltoneR = 0;
  dltoneL += (writeL - dltoneL) * dltoneCoef;
  dltoneR += (writeR - dltoneR) * dltoneCoef;

  delayBufferL[delayIndex] = dltoneL;
  delayBufferR[delayIndex] = dltoneR;
  //delayBufferL[delayIndex] = clamp(writeL, -2048, 2047) >>1; //così riduco rumore, ma ammazzo feedback
  //delayBufferR[delayIndex] = clamp(writeR, -2048, 2047) >>1;

  delayIndex = (delayIndex + 1) % DELAY_SAMPLES;

  // ===== OUTPUT TRUE STEREO =====
  procAudio_left = (int16_t)wetL;
  procAudio_right = (int16_t)((inR >> 1) + (wetR >> 1)); // dry + wet 50/50

}

void CHORUS(uint8_t slot){

  // ===== INPUT =====
  float inL = (float)deadZone(procAudio_left);
  float inR = (float)deadZone(procAudio_right);

  float lfoRate = 0.1f + ((float)FXParVal[activeChain][slot][0] / 127.0f) * 1.9f;
  static float lfoPhase = 0.0f;
  lfoPhase += lfoRate / SAMPLE_RATE;
  if(lfoPhase >= 1.0f) lfoPhase -= 1.0f;

  float lfoTri = 2.0f * fabsf(lfoPhase - 0.5f) - 1.0f;

  float depth = (float)FXParVal[activeChain][slot][1] / 127.0f;
  uint16_t modDepth = 20 + depth * 250;

  uint16_t baseDelay = 600 + (FXParVal[activeChain][slot][2] * 1000) / 127;

  int32_t delayL = baseDelay + modDepth * lfoTri;
  int32_t delayR = baseDelay - modDepth * lfoTri;

  delayL = constrain(delayL, 1, DELAY_SAMPLES-1);
  delayR = constrain(delayR, 1, DELAY_SAMPLES-1);

  uint32_t readL = (delayIndex + DELAY_SAMPLES - delayL) % DELAY_SAMPLES;
  uint32_t readR = (delayIndex + DELAY_SAMPLES - delayR) % DELAY_SAMPLES;

  int16_t wetL = delayBufferL[readL];
  int16_t wetR = delayBufferR[readR];

  delayBufferL[delayIndex] = inL;
  delayBufferR[delayIndex] = inR;

  delayIndex = (delayIndex + 1) % DELAY_SAMPLES;

  procAudio_left = (int16_t)wetL;
  procAudio_right = (int16_t)((inR / 2) + (wetR / 2)); // dry + wet 50/50
}

void FLANGER(uint8_t slot){

  // ===== INPUT =====
  float inL = deadZone(procAudio_left);
  float inR = deadZone(procAudio_right);

  float depthCtrl = (float)FXParVal[activeChain][slot][1] / 127.0f;
  float modDepth = 8.0f + depthCtrl * 128.0f;

  float fb = ((float)FXParVal[activeChain][slot][2] / 127.0f);
  fb = fb * fb * fb * 0.9f;

  static float lfoPhase = 0.0f;
  float lfoRate = 0.05f + ((float)FXParVal[activeChain][slot][0] / 127.0f) * 3.0f;
  lfoPhase += TWO_PI * lfoRate / SAMPLE_RATE;
  if(lfoPhase > TWO_PI) lfoPhase -= TWO_PI;

  float triL = 2.0f * fabsf(lfoPhase / (2*PI) - 0.5f) - 1.0f;
  float triR = -triL;

  float baseOffsetL = 32.0f;
  float baseOffsetR = 48.0f;

  float delayL = constrain(baseOffsetL + triL * modDepth, 1.0f, DELAY_SAMPLES-1);
  float delayR = constrain(baseOffsetR + triR * modDepth, 1.0f, DELAY_SAMPLES-1);

  uint32_t readL = (delayIndex >= (uint32_t)delayL) ? delayIndex - (uint32_t)delayL : DELAY_SAMPLES + delayIndex - (uint32_t)delayL;
  uint32_t readR = (delayIndex >= (uint32_t)delayR) ? delayIndex - (uint32_t)delayR : DELAY_SAMPLES + delayIndex - (uint32_t)delayR;

  float wetL = delayBufferL[readL];
  float wetR = delayBufferR[readR];

  delayBufferL[delayIndex] = (int16_t)constrain(inL + wetL * fb, -2048.0f, 2047.0f);
  delayBufferR[delayIndex] = (int16_t)constrain(inR + wetR * fb, -2048.0f, 2047.0f);

  delayIndex = (delayIndex + 1) % DELAY_SAMPLES;

  procAudio_left = (int16_t)wetL;
  procAudio_right = (int16_t)((inR / 2) + (wetR / 2)); // dry + wet 50/50
}

void REVERB(uint8_t slot){

  // ===== INPUT =====
  int32_t inL = deadZone(procAudio_left);
  int32_t inR = deadZone(procAudio_right);

  // ===== PRE-DELAY =====
  uint16_t preDelay = 200 + ((uint32_t)FXParVal[activeChain][slot][0] * 800) / 127;
  uint16_t preL = preDelay;
  uint16_t preR = preDelay + 23;   // asimmetria stereo

  // ===== DECAY =====
  float decay = 0.65f + (FXParVal[activeChain][slot][1] / 127.0f) * 0.3f;

  // ===== DAMPING =====
  float damp = 0.2f + (FXParVal[activeChain][slot][2] / 127.0f) * 0.7f;

  // ===== WIDE COMB TUNINGS =====
  const uint16_t combL[4] = { 1487, 1723, 1999, 2371 };
  const uint16_t combR[4] = { 1559, 1831, 2137, 2557 };

  int32_t accL = 0;
  int32_t accR = 0;

  for(uint8_t i=0;i<4;i++){
      uint32_t idxL = (rvbIdx + RVB_SAMPLES - combL[i] - preL) % RVB_SAMPLES;
      uint32_t idxR = (rvbIdx + RVB_SAMPLES - combR[i] - preR) % RVB_SAMPLES;

      accL += rvbBufL[idxL];
      accR += rvbBufR[idxR];
  }

  accL >>= 2;
  accR >>= 2;

  // ===== MICRO MODULATION (VERY SLOW, 1 SAMPLE) =====
  static int8_t mod = 0;
  static int8_t dir = 1;
  mod += dir;
  if(mod > 1 || mod < -1) dir = -dir;

  accL += mod;
  accR -= mod;

  // ===== DAMPING =====
  static int32_t dampL = 0;
  static int32_t dampR = 0;

  dampL += (accL - dampL) * damp;
  dampR += (accR - dampR) * damp;

  // ===== FEEDBACK WRITE =====
  rvbBufL[rvbIdx] = constrain(inL + dampL * decay, -2048, 2047);
  rvbBufR[rvbIdx] = constrain(inR + dampR * decay, -2048, 2047);

  rvbIdx++;
  if(rvbIdx >= RVB_SAMPLES) rvbIdx = 0;

  // ===== OUTPUT =====
  procAudio_left = (int16_t)dampL;                              // 100% wet
  procAudio_right = (int16_t)((inR >> 1) + (dampR >> 1));         // 50% dry + 50% wet
}

void SW_Read(){
  sw_timer++;
  if(sw_timer > 2000){ //to save resources switches are checked every tot cycles.
    sw_timer = 0;
    //BUILT-IN SWITCHES
    for (int a=0; a<4; a++){
      if (digitalRead(SW_PIN[a]) != SW_state[a]){
        SW_state[a] = !SW_state[a];
        if(SW_state[a] == LOW){//switch PRESSED, foot switch in true bypass mode
          switch (a) {
            case 0: //BUILT-IN SWITH (SW1)
              requestSave = true;
            break;
            case 1: //ENCODER 1 SW
              FX_Change(+1);
            break;
            case 2: //ENCODER 2 SW
              FX_Change(-1);
            break;
            case 4: //FOOT SW
              BYPASS = true;
            break;
          }
        }
      }
    }
  }
}

void Pot_Read() {
  //Pots value's fluctuations cause noisy effects. We then need a function to "debounce" pots.
  pot_timer++;
  if (pot_timer < 1000) return;
  pot_timer = 0;

  // ---------- CALIBRAZIONE ----------
  if (footValue_raw > footMax) {footMax = footValue_raw;}
  if (footValue_raw < footMin) {footMin = footValue_raw;}
  // protezione: range minimo valido
  if ((footMax - footMin) < 50) return;

  // ---------- SCALING ----------
  int32_t val = map(footValue_raw, footMin, footMax, 0, 4095);

  // clamp di sicurezza
  clamp (val, 0, 4095);

  // ---------- DEBOUNCE / QUANTIZZAZIONE ----------
  // riduce jitter ADC (32 step)
  footValue_q = val >> 5;

  // ---------- SOFT TAKEOVER ----------
  int8_t diff = (int8_t)footValue_q - (int8_t)FXParVal[activeChain][activeFX][0];

  if (!footPickupActive) {
    if (abs(diff) <= 3) {
      footPickupActive = true;   // pickup agganciato
    }
  }
  else{
    FXParVal[activeChain][activeFX][0] = footValue_q; //scrivi parametro
  }
}

void Encoder_Read(){
  enc_timer++;
  if(enc_timer < 20) return;
  enc_timer = 0;

  readEncoder(0, ROT_E1_PIN[0], ROT_E1_PIN[1], 5); //idx, pinA, pinB, e_step
  readEncoder(1, ROT_E2_PIN[0], ROT_E2_PIN[1], 5);

  FXParVal[activeChain][activeFX][1] = enc[0].value;
  FXParVal[activeChain][activeFX][2] = enc[1].value;
}

void FX_Change(int8_t step){
  footPickupActive = false;   //reset pickup quando cambi FX
  activeFX += step;
  if(activeFX < 0){
    activeFX = 0;
  }
  else if(activeFX >= chainLength[activeChain]){
    activeFX = chainLength[activeChain] - 1;
  }
  LoadFX();// carica nuovo FX
}

void Chain_Read() {
  activeChain = 0;
  for (uint8_t i = 0; i < 4; i++) {
    bool dipOn = (digitalRead(DIP_PIN[i]) == LOW); // INPUT_PULLUP: LOW = ON
    if (dipOn) {
        activeChain |= (1 << (3 - i));// inverte l'ordine: DIP0 fisico -> bit0 logico
    }
  }
  //activeFX = 0;// reset FX focus (Not used because this function is used in setup only).
}

void Debug(){
  deb_timer++;
  if (deb_timer > 30000){
    deb_timer = 0;

    Serial.print("ENC1 (FX): ");
    Serial.println(FXParVal[activeChain][activeFX][1]);

    Serial.print("ENC2 (FX): ");
    Serial.println(FXParVal[activeChain][activeFX][2]);

    Serial.print("EXP PEDAL (global): ");
    Serial.println(footValue_q);

    Serial.print("EXP PEDAL (FX): ");
    Serial.println(FXParVal[activeChain][activeFX][0]);

    Serial.print("CHAIN: ");
    Serial.println(activeChain);

    Serial.print("FX POSITION: ");
    Serial.println(activeFX);

    Serial.print("FOOTPICKUP: ");
    Serial.println(footPickupActive);

    Serial.print("BYPASS: ");
    Serial.println(BYPASS);

    Serial.println("");
  }
}
