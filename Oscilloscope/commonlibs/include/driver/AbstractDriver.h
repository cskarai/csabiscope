#ifndef ABSTRACTDRIVER_H_
#define ABSTRACTDRIVER_H_

#include <inttypes.h>

typedef enum
{
	TT_COMMAND,
	TT_DATA_8_BIT,
	TT_DATA_16_BIT,
	TT_DATA_32_BIT,
	TT_FLOOD_8_BIT,
	TT_FLOOD_16_BIT,
	TT_FLOOD_32_BIT,
} TransferType;

typedef void (*TransferCompletedCb)(void * arg);

class AbstractDriver;

class AbstractDriverContext
{
public:
	virtual void start(TransferType type) = 0;
	virtual void done() = 0;

	virtual int getTransferSpeed() { return -1; }

	virtual AbstractDriver * getDriver() = 0;

	virtual ~AbstractDriverContext() {};
};

class AbstractDriver
{
public:
	virtual void startDataTransfer(AbstractDriverContext * ctx, unsigned len, volatile const uint8_t * buffer, TransferType tt, TransferCompletedCb cb, void * cbArg=0) = 0;
	virtual void checkTransferCompleted() = 0;
	virtual uint32_t requiredResources() = 0;
	virtual void loop() {};

protected:
	~AbstractDriver() {}
};

#endif /* ABSTRACTDRIVER_H_ */
