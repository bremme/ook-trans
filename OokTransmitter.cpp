/*
 * RemoteSwitch library v2.3.0 (20121229) made by Randy Simons http://randysimons.nl/
 * See OokTransmitter.h for details.
 *
 * License: GPLv3. See license.txt
 */

#include "OokTransmitter.h"

/************
* OokTransmitter
************/

OokTransmitter::OokTransmitter(unsigned char pin, unsigned int pulseMicroSecond, unsigned char repeats) {
	_pin=pin;
	_pulseMicroSecond=pulseMicroSecond;
	_repeats=repeats;

	pinMode(_pin, OUTPUT);
}

unsigned long OokTransmitter::encodeTelegram(unsigned char trits[]) {
	unsigned long data = 0;

	// Encode data
	for (unsigned char i=0;i<12;i++) {
		data*=3;
		data+=trits[i];
	}

	// Encode period duration
	data |= (unsigned long)_pulseMicroSecond << 23;

	// Encode repeats
	data |= (unsigned long)_repeats << 20;

	return data;
}

void OokTransmitter::sendTelegram(unsigned char trits[]) {
	sendTelegram(encodeTelegram(trits),_pin);
}

/**
* Format data:
* pppppppp|prrrdddd|dddddddd|dddddddd (32 bit)
* p = perioud (9 bit unsigned int
* r = repeats as 2log. Thus, if r = 3, then signal is sent 2^3=8 times
* d = data
*/
void OokTransmitter::sendTelegram(unsigned long data, unsigned char pin) {
	unsigned int pulseMicroSecond = (unsigned long)data >> 23;
	unsigned char repeats = ((unsigned long)data >> 20) & B111;

	sendCode(pin, data, pulseMicroSecond, repeats);
}

void OokTransmitter::sendCode(unsigned char pin, unsigned long code, unsigned int pulseMicroSecond, unsigned char repeats, unsigned char syncPulseWidth) {
	code &= 0xfffff; // Truncate to 20 bit ;
	// Convert the base3-code to base4, to avoid lengthy calculations when transmitting.. Messes op timings.
	// Also note this swaps endianess in the process. The MSB must be transmitted first, but is converted to
	// LSB here. This is easier when actually transmitting later on.
	unsigned long dataBase4 = 0;

	for (unsigned char i=0; i<12; i++) {
		dataBase4<<=2;
		dataBase4|=(code%3);
		code/=3;
	};

	repeats = 1 << (repeats & B111); // repeats := 2^repeats;

	for (unsigned char j=0;j<repeats;j++) {
		// Sent one telegram

    // Send termination/synchronization-signal. Total length: 32 periods
    transmitSync(pin, pulseMicroSecond);

		// Recycle code as working var to save memory
		code=dataBase4;
		for (unsigned char i=0; i<12; i++) {

      transmitTrit(code & B11, pin, pulseMicroSecond);
			code>>=2;
		}
	}
}

// void OokTransmitter::transmitTrits(unsigned int tritDecCode, unsigned char codeLength, unsigned char repeats) {
//
//   quadDecCode = 0;
//
//   for (unsigned char i = 0; i < codeLength; i++) {
//     quadDecCode <<= 2;
//     quadDecCode |= tritDecCode % 3;
//     tritDecCode /= 3;
//   };
//
//   for (unsigned char r = 0; r < repeats; r++) {
//     transmitSync(pin, pulseMicroSecond, syncPulseWidth);
//     tritDecCode=quadDecCode;
//     for(unsigned char i = 0; i < codeLength; i++) {
//       transmitTrit(tritDecCode & 0x03, pin, pulseMicroSecond);
//     };
//   }
//
// };
//
// void OokTransmitter::transmitBits(unsigned long int bitDecCode, unsigned char codeLength) {
//
//   for(char i = codeLength; i >= 0; i--) {
//     transmitBit
//   }
// };



void OokTransmitter::transmitSync(unsigned char pin, unsigned int pulseMicroSecond, unsigned char syncPulseWidth) {

  digitalWrite(pin, HIGH);
  delayMicroseconds(pulseMicroSecond);
  digitalWrite(pin, LOW);
  delayMicroseconds(pulseMicroSecond*syncPulseWidth);

};

void OokTransmitter::transmitTrit(unsigned char trit, unsigned char pin, unsigned int pulseMicroSecond) {
  if ( trit == 0 ) {
    transmitBit(0, pin, pulseMicroSecond);
    transmitBit(0, pin, pulseMicroSecond);
  } else if ( trit == 1) {
    transmitBit(1, pin, pulseMicroSecond);
    transmitBit(1, pin, pulseMicroSecond);
  } else {
    transmitBit(0, pin, pulseMicroSecond);
    transmitBit(1, pin, pulseMicroSecond);
  }
};

void OokTransmitter::transmitBit(unsigned char bit, unsigned char pin, unsigned int pulseMicroSecond) {

  if(bit == 0) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(pulseMicroSecond);
    digitalWrite(pin, LOW);
    delayMicroseconds(pulseMicroSecond*3);
  } else {
    digitalWrite(pin, HIGH);
    delayMicroseconds(pulseMicroSecond*3);
    digitalWrite(pin, LOW);
    delayMicroseconds(pulseMicroSecond);
  };

};


bool OokTransmitter::isSameCode(unsigned long encodedTelegram, unsigned long receivedData) {
	return (receivedData==(encodedTelegram & 0xFFFFF)); // compare the 20 LSB's
}


/************
* ActionTransmitter
************/

ActionTransmitter::ActionTransmitter(unsigned char pin, unsigned int pulseMicroSecond, unsigned char repeats) : OokTransmitter(pin,pulseMicroSecond,repeats) {
	// Call constructor
}


void ActionTransmitter::sendSignal(unsigned char systemCode, char device, bool on) {
	sendTelegram(getTelegram(systemCode,device,on), _pin);
}

unsigned long ActionTransmitter::getTelegram(unsigned char systemCode, char device, bool on) {
	unsigned char trits[12];

	device-=65;

	for (unsigned char i=0; i<5; i++) {
		// Trits 0-4 contain address (2^5=32 addresses)
		trits[i]=(systemCode & 1)?1:2;
		systemCode>>=1;

		// Trits 5-9 contain device. Only one trit has value 0, others have 2 (float)!
		trits[i+5]=(i==device?0:2);
	}

	// Switch on or off
	trits[10]=(!on?0:2);
	trits[11]=(on?0:2);

	return encodeTelegram(trits);
}

// /************
// * BlokkerTransmitter
// ************/
//
// BlokkerTransmitter::BlokkerTransmitter(unsigned char pin, unsigned int pulseMicroSecond, unsigned char repeats) : OokTransmitter(pin,pulseMicroSecond,repeats) {
// 	// Call constructor
// }
//
//
// void BlokkerTransmitter::sendSignal(unsigned char device, bool on) {
// 	sendTelegram(getTelegram(device,on), _pin);
// }
//
// unsigned long BlokkerTransmitter::getTelegram(unsigned char device, bool on) {
// 	unsigned char trits[12]={0};
//
// 	device--;
//
// 	for (unsigned char i=1; i<4; i++) {
// 		// Trits 1-3 contain device
// 		trits[i]=(device & 1)?0:1;
// 		device>>=1;
// 	}
//
// 	// Switch on or off
// 	trits[8]=(on?1:0);
//
// 	return encodeTelegram(trits);
// }
//
// /************
// * KaKuTransmitter
// ************/
//
// KaKuTransmitter::KaKuTransmitter(unsigned char pin, unsigned int pulseMicroSecond, unsigned char repeats) : OokTransmitter(pin,pulseMicroSecond,repeats) {
// 	// Call constructor
// }
//
// void KaKuTransmitter::sendSignal(char address, unsigned char device, bool on) {
// 	sendTelegram(getTelegram(address, device, on), _pin);
// }
//
// unsigned long KaKuTransmitter::getTelegram(char address, unsigned char device, bool on) {
// 	unsigned char trits[12];
//
// 	address-=65;
// 	device-=1;
//
// 	for (unsigned char i=0; i<4; i++) {
// 		// Trits 0-3 contain address (2^4 = 16 addresses)
// 		trits[i]=(address & 1)?2:0;
// 		address>>=1;
//
// 		// Trits 4-8 contain device (2^4 = 16 addresses)
// 		trits[i+4]=(device & 1)?2:0;
// 		device>>=1;
// 	}
//
// 	// Trits 8-10 seem to be fixed
// 	trits[8]=0;
// 	trits[9]=2;
// 	trits[10]=2;
//
// 	// Switch on or off
// 	trits[11]=(on?2:0);
//
// 	return encodeTelegram(trits);
// }
//
// void KaKuTransmitter::sendSignal(char address, unsigned char group, unsigned char device, bool on) {
// 	sendTelegram(getTelegram(address, group, on), _pin);
// }
//
// unsigned long KaKuTransmitter::getTelegram(char address, unsigned char group, unsigned char device, bool on) {
// 	unsigned char trits[12], i;
//
// 	address-=65;
// 	group-=1;
// 	device-=1;
//
// 	// Address. M3E Pin A0-A3
// 	for (i=0; i<4; i++) {
// 		// Trits 0-3 contain address (2^4 = 16 addresses)
// 		trits[i]=(address & 1)?2:0;
// 		address>>=1;
// 	}
//
// 	// Device. M3E Pin A4-A5
// 	for (; i<6; i++) {
// 		trits[i]=(device & 1)?2:0;
// 		device>>=1;
// 	}
//
// 	// Group. M3E Pin A6-A7
// 	for (; i<8; i++) {
// 		trits[i]=(group & 1)?2:0;
// 		group>>=1;
// 	}
//
// 	// Trits 8-10 are be fixed. M3E Pin A8/D0-A10/D2
// 	trits[8]=0;
// 	trits[9]=2;
// 	trits[10]=2;
//
// 	// Switch on or off, M3E Pin A11/D3
// 	trits[11]=(on?2:0);
//
// 	return encodeTelegram(trits);
// }
//
//
// /************
// * ElroTransmitter
// ************/
//
// ElroTransmitter::ElroTransmitter(unsigned char pin, unsigned int pulseMicroSecond, unsigned char repeats) : OokTransmitter(pin, pulseMicroSecond, repeats) {
// 	//Call constructor
// }
//
// void ElroTransmitter::sendSignal(unsigned char systemCode, char device, bool on) {
// 	sendTelegram(getTelegram(systemCode, device, on), _pin);
// }
//
// unsigned long ElroTransmitter::getTelegram(unsigned char systemCode, char device, bool on) {
// 	unsigned char trits[12];
//
// 	device-=65;
//
// 	for (unsigned char i=0; i<5; i++) {
// 		//trits 0-4 contain address (2^5=32 addresses)
// 		trits[i]=(systemCode & 1)?0:2;
// 		systemCode>>=1;
//
// 		//trits 5-9 contain device. Only one trit has value 0, others have 2 (float)!
// 		trits[i+5]=(i==device?0:2);
// 	}
//
// 	//switch on or off
// 	trits[10]=(on?0:2);
// 	trits[11]=(!on?0:2);
//
// 	return encodeTelegram(trits);
// }
