// DBC File https://github.com/superenduro95006-coder/Volkswagen-PDC-OPS-Reader/blob/main/docs/PQ35_46_ICAN_V3_6_9_F_20081104_ASR_V1_2.dbc

#include <SPI.h>
#include <mcp_can.h>

// ---------------- Hardware ---------------- Arduino Micro (Leonardo)
static const uint8_t CAN_CS_PIN  = 9;
static const uint8_t CAN_INT_PIN = 8;
static const uint32_t CAN_SPEED  = CAN_100KBPS;//comfort bus

MCP_CAN CAN(CAN_CS_PIN);

// ---------------- IDs ----------------
static const uint32_t ID_PDC_DIST = 0x054B;
static const uint32_t ID_PDC_STAT = 0x0545;



//ggf: BO_ 1136 mBSG_Kombi: 8 Gateway_PQ35
static const uint32_t ID_REVERSE_AND_SPEED_DATA = 0x351; 
//maybe 0x271 - Byte0 bit1 
static const uint32_t ID_IGNITION_DATA = 0x575; 



// ---------------- Requests ----------------
uint8_t msg_351[8] = {0x20,0,0,0,0,0,0,0}; // GW1_Rueckfahrlicht : 1|1 → Byte0 Bit1 = 1 ⇒ Byte0 = 0x02 & GW1_FzgGeschw : 9|15 (0.01) → 0 km/h ⇒ raw = 0 ⇒ Byte1/Byte2 = 0x00 0x00
uint8_t msg_575[4] = {0x20,0,0,0}; // Byte0 (Bits 0–7) - Bit1 BS3_Klemme_15
uint8_t msg_0x470[8] = { //rueckfahrlich
    0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};
uint8_t msg_0x531[4] = { //rueckfahrlich
    0x20, 0x00, 0x00, 0x00
};
uint8_t msg_0x575[4] = { 0x02, 0x00, 0x00, 0x00 }; //Klemme 15
uint8_t msg_0x440[8] = { //rueckwaertsgang
    0x00, 0x07, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};
uint8_t msg_0x390[8] = { //rueckfahr
    0x00, 0x00, 0x02, 0x10,
    0x00, 0x00, 0x00, 0x00
};




// ---------------- State ----------------
uint32_t phase1_start = 0;
uint32_t phase2_start = 0;
uint32_t phase3_start = 0;
bool frame_sent_in_phase = false;

static const uint32_t PHASE_TIME_MS = 25;

// ---------------- Interrupt ----------------
volatile bool can_irq = false;
void onCanInt() { can_irq = true; }

// ---------------- Helpers ----------------
static void printHex2(uint8_t b) {
  if (b < 16) Serial.print('0');
  Serial.print(b, HEX);
}

static void printFrame(const char *dir, uint32_t id, uint8_t len, const uint8_t *buf) {
  Serial.print(millis());
  Serial.print(" ms ");
  Serial.print(dir);
  Serial.print(" 0x");
  Serial.print(id, HEX);
  Serial.print(" [");
  Serial.print(len);
  Serial.print("] ");
  for (uint8_t i = 0; i < len; i++) {
    printHex2(buf[i]);
    if (i + 1 < len) Serial.print(' ');
  }
  Serial.println();
}

static void sendFrame(uint32_t id, uint8_t len, const uint8_t *buf) {
  CAN.sendMsgBuf(id, 0, len, (byte*)buf);
  //printFrame("TX", id, len, buf);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  pinMode(CAN_INT_PIN, INPUT_PULLUP);

  while (CAN.begin(MCP_ANY, CAN_SPEED, MCP_8MHZ) != CAN_OK) {
    delay(300);
  }
  CAN.setMode(MCP_NORMAL);

  attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), onCanInt, FALLING);

  Serial.println("PDC Activation Run");

  phase1_start = millis();
  phase2_start = millis();
  phase3_start = millis();
}

// ---------------- Loop ----------------
void loop() {
  uint32_t now = millis();

  // -------- Phasenwechsel --------
  if (now - phase1_start >= 25) {
    phase1_start = now;
    
  }

if (now - phase2_start >= 100) {
    phase2_start = now;  
    sendFrame(ID_REVERSE_AND_SPEED_DATA, 8, msg_351);
    sendFrame(ID_IGNITION_DATA, 4, msg_575);
  }
  // -------- RX --------
  if (can_irq) can_irq = false;

  while (CAN.checkReceive() == CAN_MSGAVAIL) {
    uint32_t rxId;
    uint8_t len;
    uint8_t buf[8];
    CAN.readMsgBuf(&rxId, &len, buf);
    printFrame("RX", rxId, len, buf);
    if (rxId == ID_PDC_DIST || rxId == ID_PDC_STAT) {    
      decodeData(rxId, len, buf);
    }    
  }
}

static inline bool getBit(const byte* d, uint8_t byteIndex, uint8_t bitIndex) {
  return (d[byteIndex] >> bitIndex) & 0x01;
}

static inline uint8_t getBits(const byte* d, uint8_t byteIndex, uint8_t lsbBit, uint8_t len) {
  // len <= 8, does not cross byte boundary (hier bei Systemzustand 3 Bits innerhalb Byte7)
  uint8_t mask = (1U << len) - 1U;
  return (d[byteIndex] >> lsbBit) & mask;
}

void decodeData (uint32_t id, uint8_t len,  uint8_t *d){
   
  if (id == ID_PDC_DIST && len >= 8) {
    
  Serial.println("==== mParkhilfe_5 ====");

  // VORNE (links -> rechts)
  Serial.print("Vorne : ");
  Serial.print(d[0]);  Serial.print("  |  ");   // VL
  Serial.print(d[4]);  Serial.print("  |  ");   // VML
  Serial.print(d[5]);  Serial.print("  |  ");   // VMR
  Serial.print(d[1]);  Serial.println("");   // VR

  // HINTEN (links -> rechts)
  Serial.print("Hinten: ");
  Serial.print(d[2]);  Serial.print("  |  ");   // HL
  Serial.print(d[6]);  Serial.print("  |  ");   // HML
  Serial.print(d[7]);  Serial.print("  |  ");   // HMR
  Serial.print(d[3]);  Serial.println("");   // HR

  Serial.println("======================");
  Serial.println();
  }

  if (id == ID_PDC_STAT && len >= 8) {
    Serial.println("========== mParkhilfe_3 ==========");

  // -------- Byte 0 --------
  Serial.println("-- Systemstatus --");
  Serial.print("System codiert: "); Serial.println((d[0] & 0x01) ? "JA" : "NEIN");
  Serial.print("Anlage defekt: "); Serial.println((d[0] & 0x02) ? "JA" : "NEIN");
  Serial.print("Taster aktiviert: "); Serial.println((d[0] & 0x04) ? "JA" : "NEIN");
  Serial.print("Rueckwaertsgang aktiv: "); Serial.println((d[0] & 0x08) ? "JA" : "NEIN");
  Serial.print("Geschwindigkeit aus: "); Serial.println((d[0] & 0x10) ? "JA" : "NEIN");
  Serial.print("Tongeber vorne defekt: "); Serial.println((d[0] & 0x20) ? "JA" : "NEIN");
  Serial.print("Tongeber hinten defekt: "); Serial.println((d[0] & 0x40) ? "JA" : "NEIN");
  Serial.print("Audioabsenkung Anforderung: "); Serial.println((d[0] & 0x80) ? "JA" : "NEIN");

  // -------- Byte 1 --------
  Serial.println("\n-- Zusatzstatus --");
  Serial.print("Eis aus: "); Serial.println((d[1] & 0x01) ? "JA" : "NEIN");
  Serial.print("Schlechtweg aus: "); Serial.println((d[1] & 0x02) ? "JA" : "NEIN");
  Serial.print("KD Fehler: "); Serial.println((d[1] & 0x80) ? "JA" : "NEIN");

  // -------- Byte 2 --------
  Serial.println("\n-- Sensorstatus Hinten --");
  Serial.print("HLL defekt: "); Serial.println((d[2] & 0x01) ? "JA" : "NEIN");
  Serial.print("HLM defekt: "); Serial.println((d[2] & 0x02) ? "JA" : "NEIN");
  Serial.print("HRM defekt: "); Serial.println((d[2] & 0x04) ? "JA" : "NEIN");
  Serial.print("HRR defekt: "); Serial.println((d[2] & 0x08) ? "JA" : "NEIN");

  Serial.println("\n-- Sensorstatus Vorne --");
  Serial.print("VLL defekt: "); Serial.println((d[2] & 0x10) ? "JA" : "NEIN");
  Serial.print("VLM defekt: "); Serial.println((d[2] & 0x20) ? "JA" : "NEIN");
  Serial.print("VRM defekt: "); Serial.println((d[2] & 0x40) ? "JA" : "NEIN");
  Serial.print("VRR defekt: "); Serial.println((d[2] & 0x80) ? "JA" : "NEIN");

  // -------- Byte 3 --------
  Serial.println("\n-- Betriebsstatus --");
  Serial.print("Bereit: "); Serial.println((d[3] & 0x01) ? "JA" : "NEIN");

  uint8_t bereich_vo = (d[3] >> 2) & 0x03;
  uint8_t bereich_hi = (d[3] >> 4) & 0x03;

  Serial.print("Bereich vorne: "); Serial.println(bereich_vo);
  Serial.print("Bereich hinten: "); Serial.println(bereich_hi);

  Serial.print("Akustik aus: "); Serial.println((d[3] & 0x40) ? "JA" : "NEIN");
  Serial.print("Aufbau erkannt: "); Serial.println((d[3] & 0x80) ? "JA" : "NEIN");

  Serial.println("==================================\n");
  }
    
}
