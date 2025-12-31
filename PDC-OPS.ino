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

// Phasen
enum RevPhase : uint8_t {
  PHASE_IDLE_0 = 0,
  PHASE_351,
  PHASE_271
};

RevPhase phase = PHASE_IDLE_0;
uint32_t phase_start = 0;
bool frame_sent_in_phase = false;

static const uint32_t PHASE_TIME_MS = 200;

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
  sendFrame(ID_REQ, 2, req_1282);

  phase_start = millis();
}

// ---------------- Loop ----------------
void loop() {
  uint32_t now = millis();

  // -------- Phasenwechsel --------
  if (now - phase_start >= PHASE_TIME_MS) {
    phase_start = now;
    //Serial.println("PHASE: TEST 0x351");
    sendFrame(ID_351, 8, rev_351);
    delay (5);

    byte rev_351[8] = {0x02, 0, 0, 0, 0, 0, 0, 0};   // VW Polo: Byte0=0x02 => R
    CAN.sendMsgBuf(0x351, 0, 8, rev_351);

  // Optional: Reverse Light zusätzlich (wenn dein Setup das nutzt)
     byte revLight_390[8] = {0,0,0,0x10,0,0,0,0};  // Byte3 Bit4
     CAN.sendMsgBuf(0x390, 0, 8, revLight_390);

     byte revLight_48A[8] = {0,0,0,0x10,0,0,0,0};  // Byte3 Bit4
     CAN.sendMsgBuf(0x48A, 0, 8, revLight_48A);

    // ==========================================
    // Trailer OFF (Kandidaten – alle "0" = OFF)
    // Teste später jeweils EINEN Block, nicht alle
    // ==========================================

    // ---- Kandidat B1: 0x3C0 (Zustands-Sammelrahmen)
     byte trailerOff_3C0[8] = {0,0,0,0,0,0,0,0};
     CAN.sendMsgBuf(0x3C0, 0, 8, trailerOff_3C0);

    // ---- Kandidat B2: 0x371 (Komfortstatus, je nach Plattform)
     byte trailerOff_371[8] = {0,0,0,0,0,0,0,0};
     CAN.sendMsgBuf(0x371, 0, 8, trailerOff_371);

    // ---- Kandidat B3: 0x5A0 (Beispiel aus "Body/Trailer"-Bereich – nur als Kandidat)
     byte trailerOff_5A0[8] = {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
     CAN.sendMsgBuf(0x65A, 0, 8, trailerOff_5A0);
    
    sendFrame(ID_REQ, 2, req_1293);
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
    }
  }
}
