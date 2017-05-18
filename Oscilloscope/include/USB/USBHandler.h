#ifndef USB_USBHANDLER_H_
#define USB_USBHANDLER_H_

extern "C"
{
#include "usb_pwr.h"
#include "hw_config.h"
}

typedef enum
{
	// Good states
	STATE_NONE=0,
	STATE_TRANSMITTING,
	STATE_USER_ABORT,
	STATE_USB_ABORT,

	// Errors
	STATE_BUFFER_OVERFLOW=128,
} USBState;

typedef void (*LoopCallback)();

#define MAX_USB_TRANSMISSION_RATE 910000

#define SEND_BUFFER_SIZE        4096
#define MAX_TRANSMIT_SIZE 0x7FFFFFFF

#define FRAME_CLOSING          0
#define FRAME_DONE            -1
#define FRAME_WAITING         -2

class USBHandler
{
private:
	volatile unsigned transmittedBytes = 0;
	volatile bool     sendHeader = false;
	volatile bool     transmitting = false;
	volatile USBState usbState = STATE_NONE;

	uint8_t     * usbBuffer = 0;

	int           usbHead = 0;
	int           usbTail = 0;

	int           availableBytes = FRAME_DONE;

public:
	void          init();
	void          attach();
	void          detach();

	DEVICE_STATE  getState() { return (DEVICE_STATE)bDeviceState; }

	unsigned      getTransmittedBytes() { return transmittedBytes; }

	void          transmit( LoopCallback cb );

	void          fillUSBBuffer(int part);
	void          sendUSBBuffer();
	void          newFrame();

	bool          isTransmitting() { return transmitting; }

	USBState      getTransmissionState() { return usbState; }
	void          resetTransmissionResult() { usbState = STATE_NONE; }

	void          dataReceived(char * data, unsigned length);
};

extern USBHandler usbHandler;

#endif /* USB_USBHANDLER_H_ */
