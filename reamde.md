# PDC / OPS CAN Reverse Engineering (VW Caddy / PQ35)

## Projektziel

Ziel dieses Projekts ist es, ein **VAG Park Distance Control (PDC / OPS) Steuergerät**
(z. B. **8P0 919 475 F**) **außerhalb des Fahrzeugs auf dem Tisch** zu betreiben und
über **CAN-Bus** so anzusteuern, dass:

- das Steuergerät **Reverse erkennt**
- die **hinteren Parksensoren aktiviert** werden
- **gültige Distanzwerte** über **OPS (CAN ID 0x6DA)** ausgegeben werden

Besonderer Fokus liegt auf Steuergeräten aus **Fahrzeugen mit Anhängerbetrieb (AHK)**,
bei denen die hinteren Sensoren standardmäßig **unterdrückt** werden.

---

## Ausgangslage & Einschränkungen

- ❌ **Kein CAN-Sniffing im Fahrzeug möglich**
- ✅ Steuergerät stammt aus einem **VW Caddy (PQ35-Plattform)** mit **Anhängerzug**
- ✅ Betrieb **auf dem Tisch (Bench Setup)**
- ✅ CAN-Kommunikation über **Arduino + MCP2515**
- ⚠️ Viele Annahmen müssen **theoretisch** und über **gezielte Testmatrizen** validiert werden

---

## Verwendete Hardware

- **PDC / OPS Steuergerät**
  - z. B. `8P0 919 475 F`
- **Arduino** (Uno / Nano / kompatibel)
- **MCP2515 CAN Shield**
  - 8 MHz Quarz
- **12 V Versorgung**
  - KL15 (Zündung) + Masse
- **PDC Sensoren** (Front / Rear)
- **CAN Abschlusswiderstände (120 Ω)**

---

## CAN-Bus Grunddaten

- **CAN Speed:** `500 kbit/s`
- **Frame Typ:** Standard CAN (11-bit)
- **Bus:** je nach Fahrzeug **Antriebs-CAN oder Komfort-CAN**
  - Beim **Caddy / Nachrüstlösungen** häufig **Antriebs-CAN**

---

## Zentrale Erkenntnisse

### 1. Reverse-Signal ist notwendig, aber nicht ausreichend

Mehrere Quellen (Code, Videos, Traces) bestätigen:

- **CAN ID `0x351`**
- **Reverse = Byte0 == `0x02`**

Belegt durch:
- Skoda Fabia CAN-Code (`skoda_fabia_ms_gear_handler`)
- VW Polo Video / Screenshot
- Mehrere VAG-Plattformen (PQ25 / PQ35)

**Minimal-Reverse:**
ID : 0x351
DLC : 8
DATA: 02 00 00 00 00 00 00 00


➡️ **Allein reicht das bei AHK-Steuergeräten nicht aus.**

---

### 2. OPS / PDC arbeitet über CAN ID `0x6DA`

`0x6DA` ist ein **BAP/OPS-Multiplex-Frame**, gesendet **vom PDC selbst**.

#### Struktur (typisch):
Byte 0 : BAP Header (z. B. 0x42 oder 0x32)
Byte 1 : Block-ID
Byte 2..5 : Nutzdaten
Byte 6..7 : optional / reserviert


#### Wichtige Block-IDs

| Block | Bedeutung |
|------:|----------|
| `0x92` | Distance Front (4 Sensoren) |
| `0x93` | Distance Rear (4 Sensoren) |
| `0x94` | Presentation Control |
| `0x95` | APS Mute |
| `0x96` | OPS Display Status |
| `0x97` | Default Parking Mode |
| `0x98` | **Trailer Status** |
| `0x99` | unbekannt |

#### Beispiel: Rear Distance (ungültig)
6DA 6 42 93 FF FF FF FF


- `FF` = **kein gültiger Messwert**
- Typisch bei:
  - Rear nicht aktiv
  - Trailer-Modus aktiv
  - Sensorik nicht freigegeben

---

### 3. Front aktiv, Rear = FF → klassisches Trailer-Symptom

Beobachtung aus Logs:

- `0x92` (Front) liefert plausible Werte
- `0x93` (Rear) bleibt `FF FF FF FF`

➡️ **Sehr starkes Indiz für aktiven Anhängerbetrieb**

---

### 4. Anhängerbetrieb (AHK) unterdrückt Rear-PDC

Bei VAG-Fahrzeugen gilt:

> **Anhänger erkannt → hintere Parksensoren AUS**

Wichtig:
- Trailer-Status kommt **nicht vom PDC**
- Sondern von **AHK-Steuergerät / Gateway / BCM**
- Wird **zyklisch über CAN** gesendet

Fehlt dieser Frame auf dem Tisch:
- PDC geht oft in **Default = Anhänger vorhanden**
- Rear bleibt **dauerhaft deaktiviert**

---

## Konsequenz: Trailer OFF muss simuliert werden

Da **0x6DA Block 0x98** nur **Status** ist (Output vom PDC),
muss ein **externer CAN-Frame** gesendet werden, der bedeutet:

> **„Kein Anhänger angeschlossen“**

Ohne Sniffing ist nur eine **gezielte Kandidaten-Testreihe** möglich.

---

## Trailer-OFF Test-Kandidaten (PQ35 / Caddy)

Alle Frames:
- zyklisch (10 ms)
- DLC = 8
- Payload = **alle 0** (Default = OFF)

### Kandidaten

| Test | CAN ID | Beschreibung |
|-----:|-------:|-------------|
| T1 | `0x3C0` | BCM / Gateway Fahrzeugstatus |
| T2 | `0x35F` | BCM Status PQ35 |
| T3 | `0x371` | Komfort-/Body-Status |
| T4 | `0x390` | Reverse Light (kein Trailer, aber relevant) |

---

## Empfohlene Minimal-CAN-Umgebung (Bench)

**Pflicht:**
0x351 02 00 00 00 00 00 00 00 // Reverse ON


**Zusätzlich:**
- genau **ein** Trailer-OFF Kandidat
- zyklisch mit 10 ms

---

## Erfolgskriterium

Der Durchbruch ist erreicht, wenn:
0x6DA 42 93 xx xx xx xx


- Rear-Distanzbytes **≠ FF**
- Werte ändern sich bei Objektbewegung
- optional:
  - `0x6DA Block 0x98` zeigt „kein Anhänger“

---

## Arduino / MCP2515 – Beispiel (Reverse + Trailer-OFF Kandidat)

```cpp
byte rev_351[8] = {0x02,0,0,0,0,0,0,0};
CAN.sendMsgBuf(0x351, 0, 8, rev_351);

// Trailer OFF Kandidat (Beispiel 0x3C0)
byte trailerOff[8] = {0,0,0,0,0,0,0,0};
CAN.sendMsgBuf(0x3C0, 0, 8, trailerOff);

