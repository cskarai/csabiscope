#include "USBHandler.h"
#include "systick/SysTickHandler.h"
#include "Config.h"
#include "Sampler.h"
#include "systick/SysTime.h"
#include "InputDeviceHandler.h"
#include "xprintf.h"

#include "stm32f10x.h"

#include "xprintf.h"
#include <stdlib.h>
#include <string.h>

#define PRELL_TIME   50

extern "C"
{
#include "hw_config.h"
#include "usb_lib.h"

void DATA_RECEIVED_Callback(char * data, unsigned length)
{
	usbHandler.dataReceived(data, length);
}

void EP1_IN_Callback()
{
	usbHandler.sendUSBBuffer();
}

void SOF_Callback()
{
	if( ! usbHandler.isTransmitting() )
		return;

	usbHandler.newFrame();
}

}

USBHandler usbHandler;

void USBHandler::init()
{
	Set_System();
	USB_Cable_Config(DISABLE);
	for(volatile int i=0; i < 100000; i++);
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
}

void USBHandler::attach()
{
	USB_Interrupts_SetHighPriority();
}

void USBHandler::detach()
{
	USB_Interrupts_SetLowPriority();
}

void USBHandler::transmit( LoopCallback cb )
{
	sendHeader = true;

	usbTail = usbHead = transmittedBytes = 0;
	usbState = STATE_TRANSMITTING;

	availableBytes = FRAME_DONE;

	InputDeviceCallback oldCallback = inputDeviceHandler.getInputDeviceCallback();
	inputDeviceHandler.setInputDeviceCallback( [] (InputDeviceEvent event){
		if( event == JOY_FIRE )
			usbHandler.usbState = STATE_USER_ABORT;
	});

	usbBuffer = (uint8_t *)malloc( SEND_BUFFER_SIZE );

	Sampler::getInstance()->startTransmitter( [] (int isComplete) {
		usbHandler.fillUSBBuffer(isComplete);
	});
	Sampler::getInstance()->setTransmitterPriority(2, 0);
	inputDeviceHandler.noJoystickData();

	transmitting = true;

	while(usbState == STATE_TRANSMITTING)
	{
		inputDeviceHandler.loop();
		cb();
	}

	transmitting = false;
	while(( usbHead != usbTail ) && (availableBytes != FRAME_DONE));

	inputDeviceHandler.setInputDeviceCallback( oldCallback );

	Sampler::getInstance()->restart();

	free( usbBuffer );
	usbBuffer = 0;
}

void USBHandler::newFrame()
{
	if( availableBytes == FRAME_DONE )
	{
		availableBytes = MAX_TRANSMIT_SIZE;
		sendUSBBuffer();
	}
}

void USBHandler::sendUSBBuffer()
{
	switch( availableBytes )
	{
	case FRAME_DONE:
		return;
	case FRAME_WAITING:
		availableBytes = FRAME_DONE;
		return;
	case FRAME_CLOSING:
		CDC_Send_DATA(0, 0);
		availableBytes = FRAME_WAITING;
		return;
	default:
		break;
	}

	if( sendHeader )
	{
		char buf[BULK_MAX_PACKET_SIZE];
		int ptr = xsprintf(buf, "Bits: %d\n", (config.getTransmitTypeInt() ==  (unsigned)TRANSMIT_12_BIT_SINGLE_CHANNEL) ? 12 : 8);
		ptr += xsprintf(buf + ptr, "Channels: %d\n", config.isDualChannelTransmit() ? 2 : 1);
		ptr += xsprintf(buf + ptr, "Period: %d ns\n\n", config.getTransmitSamplingInterval() * 100 );

		transmittedBytes += ptr;
		sendHeader = false;

		CDC_Send_DATA((uint8_t *)buf, ptr);
		availableBytes = FRAME_WAITING;
		return;
	}
	else
	{
		int amount = usbHead - usbTail;
		if( amount == 0 )
		{
			availableBytes = FRAME_DONE;
			return;
		}

		if( amount < 0 )
			amount += SEND_BUFFER_SIZE;

		if( amount > BULK_MAX_PACKET_SIZE )
			amount = BULK_MAX_PACKET_SIZE;
		else
			availableBytes = amount;

		if( usbTail + amount < SEND_BUFFER_SIZE )
		{
			UserToPMABufferCopy(usbBuffer + usbTail, ENDP1_TXADDR, amount);
		}
		else
		{
			int firstFragment = SEND_BUFFER_SIZE - usbTail;
			UserToPMABufferCopy(usbBuffer + usbTail, ENDP1_TXADDR, firstFragment);
			UserToPMABufferCopy(usbBuffer, ENDP1_TXADDR + firstFragment, amount - firstFragment);
		}

		usbTail += amount;
		if( usbTail > SEND_BUFFER_SIZE )
			usbTail -= SEND_BUFFER_SIZE;

		SetEPTxCount(ENDP1, amount);
		SetEPTxValid(ENDP1);

		transmittedBytes += amount;

		availableBytes -= amount;

		if( amount != BULK_MAX_PACKET_SIZE )
			availableBytes = FRAME_WAITING;
	}
}

void USBHandler::fillUSBBuffer(int part)
{
	if( ! transmitting )
		return;

	uint32_t * samples = Sampler::getInstance()->getSampleBuffer() + (part ? SAMPLE_BUFFER_SIZE/2 : 0);
	uint32_t cnt = SAMPLE_BUFFER_SIZE/2;

	int requiredBytes;

	switch(config.getTransmitType() )
	{
	case TRANSMIT_8_BIT_DUAL_CHANNEL:
		requiredBytes = 2*cnt;
		break;
	case TRANSMIT_12_BIT_SINGLE_CHANNEL:
		requiredBytes = 3 * cnt / 2;
		break;
	case TRANSMIT_12_BIT_DUAL_CHANNEL:
		requiredBytes = 3 * cnt;
		break;
	case TRANSMIT_8_BIT_SINGLE_CHANNEL:
	default:
		requiredBytes = cnt;
		break;
	}

	int freeAmount = usbTail - usbHead;
	if( freeAmount <= 0 )
		freeAmount += SEND_BUFFER_SIZE;

	if( requiredBytes >= freeAmount )
	{
		// error
		usbState = STATE_BUFFER_OVERFLOW;
		availableBytes = FRAME_DONE;
		return;
	}

	uint8_t * buffer = usbBuffer + usbHead;
	uint8_t * buffer_end = usbBuffer + SEND_BUFFER_SIZE;

	switch(config.getTransmitType() )
	{
	case TRANSMIT_8_BIT_SINGLE_CHANNEL:
		while( cnt )
		{
#if 0
			static unsigned usbhack = 0
			*buffer++ = usbhack & 255;
			*buffer++ = usbhack >> 8;
			usbhack++;
			*buffer++ = usbhack & 255;
			*buffer++ = usbhack >> 8;
			usbhack++;
#else
			*buffer++ = ((*samples++) & 0xFFF) >> 4;
			*buffer++ = ((*samples++) & 0xFFF) >> 4;
			*buffer++ = ((*samples++) & 0xFFF) >> 4;
			*buffer++ = ((*samples++) & 0xFFF) >> 4;
#endif

			if( buffer == buffer_end )
				buffer = usbBuffer;
			cnt -= 4;
		}
		break;
	case TRANSMIT_8_BIT_DUAL_CHANNEL:
		while( cnt )
		{
			unsigned sample = *samples++;
			*buffer++ = (sample >>  4) & 0xFF;
			*buffer++ = (sample >> 20) & 0xFF;
			sample = *samples++;
			*buffer++ = (sample >>  4) & 0xFF;
			*buffer++ = (sample >> 20) & 0xFF;

			if( buffer == buffer_end )
				buffer = usbBuffer;
			cnt -= 2;
		}
		break;
	case TRANSMIT_12_BIT_SINGLE_CHANNEL:
		while( cnt )
		{
			unsigned sample = *samples++;
			*buffer++ = (sample >> 4) & 0xFF;
			unsigned nibble = (sample << 4) & 0xF0;
			sample = *samples++;
			*buffer++ = nibble | ((sample >> 8) & 255);

			if( buffer == buffer_end )
				buffer = usbBuffer;

			*buffer++ = sample & 255;
			sample = *samples++;
			*buffer++ = (sample >> 4) & 0xFF;

			if( buffer == buffer_end )
				buffer = usbBuffer;

			nibble = (sample << 4) & 0xF0;
			sample = *samples++;
			*buffer++ = nibble | ((sample >> 8) & 255);
			*buffer++ = sample & 255;

			if( buffer == buffer_end )
				buffer = usbBuffer;

			cnt-=4;
		}
		break;
	case TRANSMIT_12_BIT_DUAL_CHANNEL:
		while( cnt )
		{
			unsigned sample = *samples++;
			*buffer++ = (sample >> 4) & 0xFF;
			unsigned nibble = (sample << 4) & 0xF0;
			sample = sample >> 16;
			*buffer++ = nibble | ((sample >> 8) & 255);

			if( buffer == buffer_end )
				buffer = usbBuffer;

			*buffer++ = sample & 255;
			sample = *samples++;
			*buffer++ = (sample >> 4) & 0xFF;

			if( buffer == buffer_end )
				buffer = usbBuffer;

			nibble = (sample << 4) & 0xF0;
			sample = sample >> 16;
			*buffer++ = nibble | ((sample >> 8) & 255);
			*buffer++ = sample & 255;

			if( buffer == buffer_end )
				buffer = usbBuffer;

			cnt-=2;
		}
		break;
	}

	usbHead = buffer - usbBuffer;
}

void USBHandler::dataReceived(char * data, unsigned length)
{
	if( length >= 4 && (strncmp(data, "STOP", 4) == 0) )
	{
		if( usbState == STATE_TRANSMITTING )
			usbState = STATE_USB_ABORT;
	}
	else if( length >= 5 && (strncmp(data, "START", 5) == 0) )
	{
		if( usbState != STATE_TRANSMITTING )
		{
			data[length]=0;

			if( data[5] == ':' )
			{
				char *ptr = data + 6;

				do
				{
					long int freq = 0;

					if( !xatoi(&ptr, &freq) )
						break;

					if(( freq <= 0 ) || ( freq > MAX_USB_TRANSMISSION_RATE ))
						break;

					int interval = 10000000 / freq;
					config.setTransmitSamplingInterval(interval);

					if( *ptr++ != ',' )
						break;

					long int bits;
					if( !xatoi(&ptr, &bits) )
						break;

					if( bits != 8 && bits != 12 )
						break;

					config.setTransmitTypeInt( bits == 8 ? TRANSMIT_8_BIT_SINGLE_CHANNEL : TRANSMIT_12_BIT_SINGLE_CHANNEL );

					if( *ptr++ != ',' )
						break;

					long int channels;
					if( !xatoi(&ptr, &channels) )
						break;

					if( channels != 1 && channels != 2 )
						break;

					config.setDualChannelTransmit( channels == 2 );
				}while(false);
			}
			inputDeviceHandler.setUSBCommand(USB_COMMAND_START_TRANSMISSION);
		}
	}
}
