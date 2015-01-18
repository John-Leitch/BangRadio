/*
 * BangRadio.h
 *
 * Created: 11/24/2014 11:21:49 PM
 *  Author: John Douglas Leitch
 */ 


#ifndef RADIO_H_
#define RADIO_H_
#include <stdlib.h>
#include <string.h>
#define RadioDelayMs 1
#define RadioDelay() _delay_ms(RadioDelayMs)
#define TransmitDelayMs 5
#define TransmitDelay() _delay_ms(TransmitDelayMs)

#define MakeChunk(num, val) ((num) << 8) | (val)

#define HashInit() 0x5230
#define HashRound(h, v) ((h) ^ (v)) ^ ((v) << 11) ^ ((v) << 7) ^ ((v) << 1)
#define HashBit() 0x8000
#define HashFinalize(h) (h) | HashBit()

static inline int HashBuffer(int* buffer, int length)
{
	int hash = HashInit();

	for (int j = 0; j < length; j++)
	{
		hash = HashRound(hash, buffer[j]);
	}

	return HashFinalize(hash);
}

int i = 0;

////////////////////////////////////////////////////////////////
// Transmitter Begin
////////////////////////////////////////////////////////////////
#ifdef RadioPin && RadioPort
#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

#define ChangeRadioPin(f) f(RadioPort, RadioPin)
#define SetRadioPin() ChangeRadioPin(output_high)
#define ClearRadioPin() ChangeRadioPin(output_low)

#define Send0()			\
	SetRadioPin();		\
	RadioDelay();		\
	ClearRadioPin();	\
	RadioDelay();		\
	
#define Send1()			\
	ClearRadioPin();	\
	RadioDelay();		\
	SetRadioPin();		\
	RadioDelay();		\

#define PulseRadioPin()	\
SetRadioPin();			\
RadioDelay();			\
ClearRadioPin();		\
RadioDelay();			\

char tx8 = 0;
int tx16 = 0;

#define WriteBits(txBits, bitCount)	\
	for (i = 0; i < bitCount; i++)	\
	{                               \
		if (txBits & 1)				\
		{                           \
			Send1();				\
		}							\
		else						\
		{							\
			Send0();				\
		}							\
									\
		txBits >>= 1;				\
	}								\

#define Write8() WriteBits(tx8, 8)
#define Write16() WriteBits(tx16, 16)

static inline void WriteBuffer(char* buffer, int length)
{
	int hash = HashInit();
	
	for (int j = 0; j < length; j++)
	{
		tx16 = MakeChunk(j + 1, buffer[j]);
		hash = HashRound(hash, tx16);
		Write16();
		TransmitDelay();
	}
	
	tx16 = HashFinalize(hash);
	Write16();
	TransmitDelay();
}

#define ChunkSize 2
#define ManchesterBitSize 2
#define RadioBufferSizeOf(length) ((length) * (ChunkSize) * (ManchesterBitSize))
#define RadioBufferTransmitTimeOf(length) ((RadioBufferSizeOf(((length) + 1)) * (RadioDelayMs)) + (((length) + 1) * TransmitDelayMs))

#endif
////////////////////////////////////////////////////////////////
// Transmitter End
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Receiver Begin
////////////////////////////////////////////////////////////////
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)

char bit0, bit1;
char rx8 = 0;
int rx16 = 0;

#define ReceiverPin 8
#define GetReceiverBit() PINB & 1

#define ReadBits(rxBits, bitCount)	\
	rxBits = 0;						\
									\
	for (i = 0; i < bitCount; i++)	\
	{								\
		bit0 = GetReceiverBit();	\
		RadioDelay();				\
		bit1 = GetReceiverBit();	\
		RadioDelay();				\
									\
		if (bit0 == bit1)			\
		{							\
			rxBits = -1;			\
			break;					\
		}							\
									\
		rxBits |= (bit1 << i);		\
	}								\
									\

#define Read8() ReadBits(rx8, 8)
#define Read16() ReadBits(rx16, 16)

static inline bool ReadBuffer(int* packet, int length)
{
	char* chunks = (char*)malloc(length);
	memset(chunks, 0, length);	
	short currentChunk = 0;
	bool waiting = true;
	int j = 0;
	
	while (waiting)
	{
		Read16();

		if (rx16 != -1)
		{
			if (rx16 & HashBit())
			{
				currentChunk = length - 1;
			}
			else
			{
				currentChunk = (rx16 >> 8) - 1;

				if (currentChunk > length - 2)
				{
					continue;
				}
			}
			
			packet[currentChunk] = rx16;
			chunks[currentChunk] = 1;
			waiting = false;

			for (j = 0; j < length; j++)
			{
				if (!chunks[j])
				{
					waiting = true;
					break;
				}
			}
		}
	}
	
	currentChunk = length - 1;
	int hash = HashBuffer(packet, currentChunk);	
	free(chunks);
	
	return hash == packet[currentChunk];
}

#endif
////////////////////////////////////////////////////////////////
// Receiver End
////////////////////////////////////////////////////////////////
#endif /* RADIO_H_ */