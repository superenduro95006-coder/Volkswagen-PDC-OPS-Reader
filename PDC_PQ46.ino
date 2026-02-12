// DBC File https://github.com/commaai/opendbc/blob/master/opendbc/dbc/vw_pq.dbc

#include <SPI.h>
#include <mcp_can.h>

// ---------------- Hardware ----------------
static const uint8_t CAN_CS_PIN  = 9;
static const uint8_t CAN_INT_PIN = 8;
static const uint32_t CAN_SPEED  = CAN_500KBPS;

MCP_CAN CAN(CAN_CS_PIN);

// ---------------- IDs ----------------
static const uint32_t ID_PDC_RESPONSE = 0x054B;

static const uint32_t ID_REVERSE_DATA = 0x390;
static const uint32_t ID_SPEED_DATA = 0x320;


// ---------------- Requests ----------------
uint8_t msg_320[8] = {0,0,0,0,0,0,0,0};
uint8_t msg_390[8] = {0,0,0x02,0x10,0,0,0,0}; 
// Kein Anhänger
// GK1_RueckfahrSch = 1 (Byte 2 - Bit1)
// GK1_Rueckfahr = 1 (Byte 3 - Bit4)


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

  calculateSpeed(0, msg_320);

}

// ---------------- Loop ----------------
void loop() {
  uint32_t now = millis();

  // -------- Phasenwechsel --------
  if (now - phase1_start >= 25) {
    phase1_start = now;
    sendFrame(ID_SPEED_DATA, 8, msg_320);
  }

if (now - phase2_start >= 100) {
    phase2_start = now;  
    sendFrame(ID_REVERSE_DATA, 8, msg_390);
  }
  // -------- RX --------
  if (can_irq) can_irq = false;

  while (CAN.checkReceive() == CAN_MSGAVAIL) {
    uint32_t rxId;
    uint8_t len;
    uint8_t buf[8];
    CAN.readMsgBuf(&rxId, &len, buf);

    if (rxId == ID_PDC_RESPONSE) {
        //printFrame("RX", rxId, len, buf);
      decodeData(rxId, len, buf);
    }
  }
}

void calculateSpeed(float speed_kmh, uint8_t *d) {
  
  uint16_t raw_speed = (uint16_t)(speed_kmh / 0.32); // = 31

  //SG_ Angezeigte_Geschwindigkeit : 46|10@1+
  //
  // Startbit 46:
  //  - Byte5 Bit6–7  -> obere 2 Bits
  //  - Byte6 Bit0–7  -> untere 8 Bits
  d[5] |= (raw_speed & 0x03) << 6;   // Bits 0–1 -> Byte5 Bit6–7
  d[6] |= (raw_speed >> 2) & 0xFF;   // Bits 2–9 -> Byte6
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
   
  if (id == ID_PDC_RESPONSE && len >= 8) {
    uint8_t hinten_links  = d[2]; // Byte 3
    uint8_t hinten_rechts = d[3]; // Byte 4
    uint8_t hinten_mitte_links  = d[6]; // Byte 7
    uint8_t hinten_mitte_rechts = d[7]; // Byte 8;

    Serial.print("HL: ");
    printHex2(hinten_links);
    Serial.print(" HML: ");
    printHex2(hinten_mitte_links);
    Serial.print(" HMR: ");
    printHex2(hinten_mitte_rechts);
    Serial.print(" HR: ");
    printHex2(hinten_rechts);
    Serial.println("");
  }
}
