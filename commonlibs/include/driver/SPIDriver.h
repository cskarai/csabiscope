#ifndef SPIDRIVER_H_
#define SPIDRIVER_H_

#include "driver/AbstractDriver.h"
#include "pins.h"

class SPIDriver : virtual public AbstractDriver
{
private:
	unsigned                spiNum;
	SPI_TypeDef           * spi;
	DMA_Channel_TypeDef   * dma;

	AbstractDriverContext * ctx;
	TransferCompletedCb     callback;
	void                  * callbackArg;

	void initRCC();
	void initSPI();
	void initNVIC();
	void initDMA();
	void configureDMA(TransferType tt);

	void transferDone();

public:
	SPIDriver(unsigned spiNum);

	AbstractDriverContext * createSPIContext(STM32Pin cs, int prescaler);
	AbstractDriverContext * createSPILCDContext(STM32Pin cs, STM32Pin a0, STM32Pin backlight, int prescaler);

	void init();
	virtual void startDataTransfer(AbstractDriverContext * ctx, unsigned len, volatile const uint8_t * buffer, TransferType tt, TransferCompletedCb cb, void * cbArg=0);
	virtual void checkTransferCompleted();
	virtual uint32_t requiredResources();

	virtual ~SPIDriver() {}
};

#endif /* SPIDRIVER_H_ */
