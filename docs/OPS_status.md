```
ich habe hier noch etwas gefunden: 0x497
BO_ 1175
SG_ PH_Abschaltursache : 13|3@1+ (1,0) [0|7] "" Rearview
SG_ PH_Opt_Anzeige_V_ein : 16|1@1+ (1,0) [0|1] "" Rearview
SG_ PH_Opt_Anzeige_H_ein : 17|1@1+ (1,0) [0|1] "" Rearview
SG_ PH_Opt_Anz_V_Hindernis : 18|1@1+ (1,0) [0|1] "" Vector__XXX
SG_ PH_Opt_Anz_H_Hindernis : 19|1@1+ (1,0) [0|1] "" Vector__XXX
SG_ PH_Tongeber_V_aktiv : 20|1@1+ (1,0) [0|1] "" Vector__XXX
SG_ PH_Tongeber_H_aktiv : 21|1@1+ (1,0) [0|1] "" Vector__XXX
SG_ PH_Tongeber_mute : 22|1@1+ (1,0) [0|1] "" Vector__XXX
SG_ PH_Anf_Audioabsenkung : 23|1@1+ (1,0) [0|1] "" Radio_Modul,RNS_300_NF,RNS_midline
SG_ PH_Frequenz_hinten : 32|4@1+ (1,0) [0|15] "" Vector__XXX
SG_ PH_Lautstaerke_hinten : 36|4@1+ (1,0) [0|15] "" Vector__XXX
SG_ PH_Frequenz_vorn : 40|4@1+ (1,0) [0|15] "" Vector__XXX
SG_ PH_Lautstaerke_vorn : 44|4@1+ (1,0) [0|15] "" Vector__XXX
SG_ PH_Trigger_Bildaufschaltung : 48|1@1+ (1,0) [0|1] "" Vector__XXX
SG_ PH_StartStopp_Info : 49|2@1+ (1,0) [0|3] "" Vector__XXX
SG_ PH_Aufbauten_erk : 51|1@1+ (1,0) [0|1] "" Vector__XXX
SG_ PH_BerErk_vorn : 52|2@1+ (1,0) [0|3] "" Vector__XXX
SG_ PH_BerErk_hinten : 54|2@1+ (1,0) [0|3] "" Verdeck
SG_ PH_defekt : 56|1@1+ (1,0) [0|1] "" Verdeck
SG_ PH_gestoert : 57|1@1+ (1,0) [0|1] "" Verdeck
SG_ PH_Systemzustand : 58|3@1+ (1,0) [0|7] "" AAG,Rearview,RNS_midline
SG_ PH_Display_Kundenwunsch : 61|2@1+ (1,0) [0|3] "" Vector__XXX
SG_ PH_KD_Fehler : 63|1@1+ (1,0) [0|1] "" Vector__XXX

0x6DA
BO_ 1754
SG_ BAP_Data_OPS : 0|16@1+ (1,0) [0|65535] "" Radio_Modul,RNS_300_NF,RNS_midline
```

**1) 0x497: Bit-/Byte-Mapping aus deinem DBC (Intel / little endian)**

Dein DBC sagt @1 → Intel. Dann gilt:

Byte = Startbit / 8

Bit im Byte = Startbit % 8 (LSB=Bit0)

Hier die wichtigsten Signale in Byte/Bit:

```
Aktiv/Anzeige/Audio (die du wirklich sehen willst)

PH_Opt_Anzeige_V_ein Startbit 16, Len1 → Byte2 Bit0

PH_Opt_Anzeige_H_ein Startbit 17, Len1 → Byte2 Bit1

PH_Tongeber_V_aktiv Startbit 20, Len1 → Byte2 Bit4

PH_Tongeber_H_aktiv Startbit 21, Len1 → Byte2 Bit5

PH_Tongeber_mute Startbit 22, Len1 → Byte2 Bit6

PH_Trigger_Bildaufschaltung Startbit 48, Len1 → Byte6 Bit0

PH_Systemzustand Startbit 58, Len3 → Byte7 Bits2..4

PH_defekt Startbit 56, Len1 → Byte7 Bit0

PH_gestoert Startbit 57, Len1 → Byte7 Bit1

PH_Abschaltursache Startbit 13, Len3 → Byte1 Bits5..7

Sound-Parameter (Nibbles)

PH_Frequenz_hinten Startbit 32, Len4 → Byte4 low nibble (Bits0..3)

PH_Lautstaerke_hinten Startbit 36, Len4 → Byte4 high nibble (Bits4..7)

PH_Frequenz_vorn Startbit 40, Len4 → Byte5 low nibble

PH_Lautstaerke_vorn Startbit 44, Len4 → Byte5 high nibble
```

Das reicht, um sehr schnell zu sehen: ist das System “aktiv hinten”, “aktiv vorne”, will es Bildaufschaltung, ist es gestört/defekt.

**2) 0x6DA: “BAP_Data_OPS”**

```
für das 0x6DA Byte mapping habe ich folgendes gefunden: Function         Example
            02 82 03 00 0a 00 03 00
            32 82 03 00 0a 00 03 00
versionControlStatus      32 83 38 03 fb 40 00 00
            32 84 0a
fSG_SetupStatus         32 8e 0f 03 03
fSG_OperationStateStatu      32 8f 00
Sound front         32 90 06 04
Sound rear         32 91 04 04
Distance front         32 92 ff ff ff ff
Distance rear         32 93 ff ff ff ff
Presentation control      32 94 00 00
OpsDisplayStatusEvent      32 96 00
Default parking mode?      32 97 01
?            32 99 00
Trailer            32 98 00
APS mute         32 95 00
```


| Block | Bedeutung                       |
| ----: | ------------------------------- |
|  0x82 | unknown / base status           |
|  0x83 | versionControlStatus            |
|  0x84 | unknown                         |
|  0x8E | fSG_SetupStatus                 |
|  0x8F | fSG_OperationStateStatus        |
|  0x90 | Sound front                     |
|  0x91 | Sound rear                      |
|  0x92 | **Distance front (4 Sensoren)** |
|  0x93 | **Distance rear (4 Sensoren)**  |
|  0x94 | Presentation control            |
|  0x95 | APS mute                        |
|  0x96 | OPS Display Status Event        |
|  0x97 | Default parking mode            |
|  0x98 | **Trailer status**              |
|  0x99 | unknown                         |

