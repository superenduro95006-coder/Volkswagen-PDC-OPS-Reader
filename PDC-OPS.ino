// DBC File https://github.com/commaai/opendbc/blob/master/opendbc/dbc/vw_pq.dbc

#include <SPI.h>
#include <mcp_can.h>

// ---------------- Hardware ----------------
static const uint8_t CAN_CS_PIN  = 9;
static const uint8_t CAN_INT_PIN = 8;
static const uint32_t CAN_SPEED  = CAN_500KBPS;

MCP_CAN CAN(CAN_CS_PIN);

// ---------------- IDs ----------------
static const uint32_t ID_REQ  = 0x67A;
static const uint32_t ID_RESP = 0x6DA;

static const uint32_t ID_351 = 0x351;
static const uint32_t ID_390 = 0x390;
static const uint32_t ID_65A = 0x65A;
static const uint32_t ID_271 = 0x271;


// ---------------- Requests ----------------
uint8_t req_1282[2] = { 0x12, 0x82 };
uint8_t req_1281[2] = { 0x12, 0x81 };
uint8_t req_1293[2] = { 0x12, 0x93 };
uint8_t req_1294[4] = { 0x12, 0x94, 0xFF, 0x01 };
uint8_t req_1296[4] = { 0x12, 0x96, 0x01, 0x00 };



// Reverse Frames
uint8_t rev_351[8] = { 0x02, 0x01,0,0,0,0,0x7F,0 };   // Byte0 Bit2
uint8_t rev_390[8] = { 0x00, 0,0,0x16,0,0,0,0 };
uint8_t rev_65A[8] = { 0x01, 0,0,0,0,0,0,0 };
uint8_t rev_271[1] = { 0x07};


// ---------------- State ----------------
bool sent_81 = false;


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

  Serial.println("OPS Reverse One-Shot Test gestartet");

  // Initialer Handshake
  //sendFrame(ID_REQ, 2, req_1282);

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

    byte msg_320[8] = {0,0,0,0,0,0,0,0}; //{0};

// Beispielgeschwindigkeit
float speed_kmh = 12.0;
uint16_t raw_speed = (uint16_t)(speed_kmh / 0.32); // = 31

// SG_ Angezeigte_Geschwindigkeit : 46|10@1+
//
// Startbit 46:
//  - Byte5 Bit6–7  -> obere 2 Bits
//  - Byte6 Bit0–7  -> untere 8 Bits

//msg_320[4] |= 0x01;
//msg_320[5] |= (raw_speed & 0x03) << 6;   // Bits 0–1 -> Byte5 Bit6–7
//msg_320[6] |= (raw_speed >> 2) & 0xFF;   // Bits 2–9 -> Byte6


    CAN.sendMsgBuf(0x320, 0, 8, msg_320);

  }

if (now - phase2_start >= 100) {
    phase2_start = now;  



byte msg_390[8] = {0};

msg_390[0] = 0x00;          // Kein Anhänger
msg_390[2] = 0x02;          // GK1_RueckfahrSch = 1 (Bit1)
msg_390[3] = 0x10;          // GK1_Rueckfahr = 1 (Bit4)

CAN.sendMsgBuf(0x390, 0, 8, msg_390);
delay(5);



byte msg_570[4] = {0};

// ZAS_Klemme_15 : 1|1@1+  -> Byte0 Bit1
msg_570[0] |= 0x02;   // KL15 EIN

CAN.sendMsgBuf(0x570, 0, 4, msg_570);




  }

  if (now - phase3_start >= 400) {
    phase3_start = now;  

    byte msg_9296[8] = {0x42, 0x96, 0x01, 0x01};
    sendFrame(ID_REQ, 4, msg_9296);
  }

  // -------- RX --------
  if (can_irq) can_irq = false;

  while (CAN.checkReceive() == CAN_MSGAVAIL) {
    uint32_t rxId;
    uint8_t len;
    uint8_t buf[8];
    CAN.readMsgBuf(&rxId, &len, buf);

    
    
    if (rxId == ID_RESP) {
      printFrame("RX", rxId, len, buf);

      // ACK → 12 81 einmalig
      if (!sent_81 && len >= 2 && buf[1] == 0x82) {
        sendFrame(ID_REQ, 2, req_1281);
        sent_81 = true;
      }

      decodeData(rxId, buf);


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

static void printHex8(const byte* d) {
  for (int i = 0; i < 8; i++) {
    if (d[i] < 0x10) Serial.print("0");
    Serial.print(d[i], HEX);
    if (i < 7) Serial.print(" ");
  }
}

void decodeData (uint32_t id, uint8_t *d){
   
  if (id == 0x497) {
    bool optV = getBit(d, 2, 0);  // Startbit 16
    bool optH = getBit(d, 2, 1);  // Startbit 17
    bool toneV = getBit(d, 2, 4); // Startbit 20
    bool toneH = getBit(d, 2, 5); // Startbit 21
    bool mute  = getBit(d, 2, 6); // Startbit 22

    bool trigBild = getBit(d, 6, 0); // Startbit 48

    uint8_t sysZ = getBits(d, 7, 2, 3); // Startbit 58 len3 => Byte7 bits2..4
    bool defekt   = getBit(d, 7, 0);    // 56
    bool gestoert = getBit(d, 7, 1);    // 57

    uint8_t abschalt = getBits(d, 1, 5, 3); // Startbit 13 => Byte1 bits5..7

    uint8_t freqH = (d[4] & 0x0F);        // Startbit 32 len4
    uint8_t volH  = (d[4] >> 4) & 0x0F;   // Startbit 36 len4
    uint8_t freqV = (d[5] & 0x0F);        // Startbit 40 len4
    uint8_t volV  = (d[5] >> 4) & 0x0F;   // Startbit 44 len4

    Serial.print("0x497  RAW: "); printHex8(d);
    Serial.print(" | OptV="); Serial.print(optV);
    Serial.print(" OptH="); Serial.print(optH);
    Serial.print(" ToneV="); Serial.print(toneV);
    Serial.print(" ToneH="); Serial.print(toneH);
    Serial.print(" Mute="); Serial.print(mute);
    Serial.print(" Bild="); Serial.print(trigBild);
    Serial.print(" Sys="); Serial.print(sysZ);
    Serial.print(" Def="); Serial.print(defekt);
    Serial.print(" Gst="); Serial.print(gestoert);
    Serial.print(" Abschalt="); Serial.print(abschalt);
    Serial.print(" FreqH/VolH="); Serial.print(freqH); Serial.print("/"); Serial.print(volH);
    Serial.print(" FreqV/VolV="); Serial.print(freqV); Serial.print("/"); Serial.println(volV);
  }

  if (id == 0x6DA) {
    handle_6DA(d);
  }
}

void handle_6DA(const byte* d) {
  byte header = d[0];
  byte block  = d[1];

  Serial.print("0x6DA  RAW: ");
  printHex8(d);

  Serial.print(" | Hdr=0x");
  Serial.print(header, HEX);
  Serial.print(" Block=0x");
  Serial.print(block, HEX);
  Serial.print(" -> ");

  switch (block) {

    case 0x82:
      Serial.print("Base/Unknown Status");
      break;

    case 0x83:
      Serial.print("VersionControlStatus ");
      Serial.print("v=");
      Serial.print(d[2], HEX);
      Serial.print(".");
      Serial.print(d[3], HEX);
      break;

    case 0x84:
      Serial.print("Unknown (0x84)");
      break;

    case 0x8E:
      Serial.print("SetupStatus ");
      Serial.print("Flags=");
      Serial.print(d[2], HEX);
      Serial.print(" Mode=");
      Serial.print(d[3], HEX);
      break;

    case 0x8F:
      Serial.print("OperationStateStatus ");
      Serial.print("State=");
      Serial.print(d[2], HEX);
      break;

    case 0x90:
      Serial.print("Sound Front ");
      Serial.print("Freq=");
      Serial.print(d[2]);
      Serial.print(" Vol=");
      Serial.print(d[3]);
      break;

    case 0x91:
      Serial.print("Sound Rear ");
      Serial.print("Freq=");
      Serial.print(d[2]);
      Serial.print(" Vol=");
      Serial.print(d[3]);
      break;

    case 0x92:
      Serial.print("Distance FRONT [");
      Serial.print(d[2], HEX); Serial.print(" ");
      Serial.print(d[3], HEX); Serial.print(" ");
      Serial.print(d[4], HEX); Serial.print(" ");
      Serial.print(d[5], HEX);
      Serial.print("]");
      break;

    case 0x93:
      Serial.print("Distance REAR  [");
      Serial.print(d[2], HEX); Serial.print(" ");
      Serial.print(d[3], HEX); Serial.print(" ");
      Serial.print(d[4], HEX); Serial.print(" ");
      Serial.print(d[5], HEX);
      Serial.print("]");
      break;

    case 0x94:
      Serial.print("PresentationControl ");
      Serial.print(d[2], HEX);
      Serial.print(" ");
      Serial.print(d[3], HEX);
      break;

    case 0x95:
      Serial.print("APS Mute ");
      Serial.print(d[2] ? "ON" : "OFF");
      break;

    case 0x96:
      Serial.print("OPS Display Status ");
      Serial.print(d[2], HEX);
      break;

    case 0x97:
      Serial.print("Default Parking Mode ");
      Serial.print(d[2], HEX);
      break;

    case 0x98:
      Serial.print("Trailer Status ");
      Serial.print(d[2] == 0x00 ? "NO TRAILER" : "TRAILER ACTIVE");
      break;

    case 0x99:
      Serial.print("Unknown (0x99)");
      break;

    default:
      Serial.print("Unknown Block");
      break;
  }

  Serial.println();
}
