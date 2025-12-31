For those still following along, some notes on the CAN format. It's somewhat complicated to allow for messages longer than 8 bytes, and is a shared protocol between OPS, the official Audi reversing camera, and AMI.

```
First 2 bytes:
0---------------	Basic packet flag (6 bytes or less)
0xxx------------	Some kind of type?
0---xxxxxx------	Device: 0A=OPS, 0B=Reverse Camera, 2F=AMI
0---------xxxxxx	Packet ID: Device dependent
Followed by up to 6 bytes of packet data

Or:
1---------------	Extended packet flag (more than 6 bytes)
10--------------	Start packet indicator
10xx------------	Pipe id? For 4 separate pipes
10--xxxxxxxxxxxx	Extended packet data length
Followed by 2 bytes as per basic packet (type/device/id)
Followed by first 4 bytes of packet data

Followed by continuation CAN packets:
11------			Continuation packet indicator
11xx----			Matches pipe ID of  first packet
11--xxxx			Sequence counter
```
Ok, so now we have that straight, for a front+rear module we get the following data over CAN...
```
Basic packets:
6DA: 02 82 03 00 0A 00 03 00	02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes
6DA: 42 82 03 00 0A 00 03 00	02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes

Extended packet:
6DA: 80 24 42 81 03 00 0A 00
6DA: C0 03 00 38 03 FB 80 00
6DA: C1 00 00 00 14 1F 03 03
6DA: C2 00 06 04 04 06 FF FF
6DA: C3 FF FF FF FF FF FF 00
6DA: C4 00 00 01 00

The above extended packet gets decoded by the 03 flags packet to the following packets:
								02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes
								03 = 38 03 FB 80 00 00 00 00	Flags - indicate supplied packet IDs
								04 = 14							Some kind of timeout value
								0E = 1F 03 03					First byte must have 4 low bits set to display OPS
																Second byte flags for front vol/freq supported
																Third byte flags for rear vol/freq supported
								0F = 00							00=vol/freq adjustment enabled
								10 = 06 04						Front vol/freq setting
								11 = 04 06						Rear vol/freq setting
								14 = 00 00						OPS off?
								16 = 00							00=Don't display OPS
								17 = 01							Unknown

Basic packets:
6DA: 32 82 03 00 0A 00 03 00	02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes
6DA: 32 83 38 03 FB 80 00 00	03 = 38 03 FB 80 00 00			Seems to be a duplicate of extended packet flags?
6DA: 42 96 01					16 = 01							01=Display OPS			
6DA: 42 94 FF 01				14 = FF 01						OPS on?
6DA: 42 93 FF FF FF FF			13 = FF FF FF FF				Rear sensors (L - LC - RC - R)
6DA: 42 92 FF FF FF FF			12 = FF FF FF FF				Front sensors (L - LC - RC - R)
```
And now looking at the rear only data as kindly provided by Llewkcalb:
```
Basic packets:
6DA: 02 82 03 00 0A 00 03 00	02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes
6DA: 42 82 03 00 0A 00 03 00	02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes

Extended packets:
6DA: 80 1D 42 81 03 00 0A 00
6DA: C0 03 00 38 03 5B 00 00
6DA: C1 00 00 00 14 0A 00 03
6DA: C2 00 04 03 FF FF FF FF
6DA: C3 00 00 00 01

The above extended packet gets decoded by the 03 flags packet to the following packets:
								02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes
								03 = 38 03 5B 00 00 00 00 00	Flags - indicate supplied packet IDs
								04 = 14							Some kind of timeout value
								0E = 0A 00 03					First byte must have 4 low bits set to display OPS
																Second byte flags for front vol/freq supported
																Third byte flags for rear vol/freq supported
								0F = 00							00=vol/freq adjustment enabled
								11 = 04 03						Rear vol/freq setting
								14 = 00 00						OPS off?
								16 = 00							00=Don't display OPS
								17 = 01							Unknown

Basic packets:
6DA: 32 82 03 00 0A 00 03 00	02 = 03 00 0A 00 03 00			Version?  RNS-E checks for these exact bytes
6DA: 32 83 38 03 5B 00 00 00	03 = 38 03 5B 00 00 00			Seems to be a duplicate of extended packet flags?
6DA: 32 84 14					04 = 14							Some kind of timeout value
6DA: 32 8E 0A 00 03				0E = 0A 00 03					Seems to be a duplicate of extended packet?
6DA: 32 8F 00					0F = 00							00=vol/freq adjustment enabled
6DA: 42 96 01					16 = 00							00=Don't display OPS
6DA: 42 94 FF 01				14 = FF 01						OPS on?							
6DA: 42 93 FF FF FF FF			13 = FF FF FF FF				Rear sensors (L - LC - RC - R)
```
Things to notice:
1. Packet type 0x10 for front vol/freq setting is not sent. The RNS-E expects this packet type or else nothing is displayed.
2. Packet type 0x12 for front sensor data is not sent. The RNS-E expects this packet type or else nothing is displayed.
3. Packet type 0x0E starts with byte 0x0A instead of 0x1F. This is what is causing the park assist mode to display, instead of OPS. The bottom 4 bits need to be set for OPS to display.

Edit: Some more details on the decoding of the extended packet with packet ID 01
This seems to consist of a mandatory type 02 packet with the fixed data: 03 00 0A 00 03 00
Followed by a mandatory type 03 packet: 38 03 FB 80 00 00 00 00 or 38 03 5B 00 00 00 00 00 in the examples above
The data in the 03 packet is a sequence of bit flags with each flag indicating which of the other packet IDs are included in the remaining data (actually there are flags for the supposedly mandatory 02 03 packet IDs as well, but it's not clear how they could be used due to the flags needing to exist at a defined location in the extended packet). So...
3803FB80... = 02 03 04 0E 0F 10 11 12 13 14 16 17 18
38035B00... = 02 03 04 0E 0F 11 13 14 16 17
Note that for some reason some packet IDs are supplied (12, 13 & 18) but just skipped by the firmware. Instead the firmware waits for the basic packet format of these IDs to provide the data.



Source: https://www.ttforum.co.uk/threads/rns-e-firmware-now-including-32gb-sdhc-support-for-192.2003957/page-3?nested_view=1
