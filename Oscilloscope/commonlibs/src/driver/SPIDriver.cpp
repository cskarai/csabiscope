#include "driver/SPIDriver.h"
#include "pins.h"
#include "task/Resource.h"

#if defined SPI1_DMA1_IRQ_HANDLER || defined SPI2_DMA1_IRQ_HANDLER
static SPIDriver * spiDriver;
void (*irqCallback)(uint8_t spi);
#endif

#ifdef SPI1_DMA1_IRQ_HANDLER

extern "C" void DMA1_Channel3_IRQHandler()
{
	if(DMA_GetITStatus(DMA1_IT_TC3))
	{
		DMA_ClearITPendingBit(DMA1_IT_GL3);

		if( irqCallback != 0 )
			irqCallback(1);
	}
}

#endif

#ifdef SPI2_DMA1_IRQ_HANDLER

extern "C" void DMA1_Channel5_IRQHandler()
{
	if(DMA_GetITStatus(DMA1_IT_TC5))
	{
		DMA_ClearITPendingBit(DMA1_IT_GL5);

		if( irqCallback != 0 )
			irqCallback(2);
	}
}

#endif

SPIDriver::SPIDriver(unsigned spiNumIn) : spiNum(spiNumIn), spi(0), dma(0), ctx(0), callback(0), callbackArg(0)
{
}

void SPIDriver::init()
{
	STM32Pin sck, miso, mosi;

	if( spiNum == 1 )
	{
		spi = SPI1;
		dma = DMA1_Channel3;

		sck = PA5;
		miso = PA6;
		mosi = PA7;
	}
	if( spiNum == 2 )
	{
		spi = SPI2;
		dma = DMA1_Channel5;

		sck = PB13;
		miso = PB14;
		mosi = PB15;
	}

	enableRCC(sck);
	enableRCC(mosi);
	enableRCC(miso);
	pinMode(sck, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
	pinMode(mosi, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
	pinMode(miso, GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz);

	initRCC();
	initSPI();
	initDMA();
	initNVIC();
}

void SPIDriver::initRCC()
{
	if( spiNum == 1 )
	{
		/* PCLK2 = HCLK/2 */
		RCC_PCLK2Config(RCC_HCLK_Div1);

		/* Enable GPIO clock for SPIz */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

		/* Enable SPI Periph clock */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	}
	else if( spiNum == 2 )
	{
		/* PCLK2 = HCLK/2 */
		RCC_PCLK1Config(RCC_HCLK_Div1);

		/* Enable GPIO clock for SPIz */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

		/* Enable SPI Periph clock */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	}

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

void SPIDriver::initSPI()
{
	SPI_InitTypeDef   SPI_InitStructure;

	/* SPI Config -------------------------------------------------------------*/
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(spi, &SPI_InitStructure);

	/* Enable SPI */
	SPI_Cmd(spi, ENABLE);

	/* Enable the SPI Tx DMA requests */
	SPI_I2S_DMACmd(spi, SPI_I2S_DMAReq_Tx, ENABLE);
}

void SPIDriver::initNVIC()
{
#if defined SPI1_DMA1_IRQ_HANDLER || defined SPI2_DMA1_IRQ_HANDLER
	spiDriver = this;
	irqCallback = [] (uint8_t) {
		spiDriver->transferDone();
	};

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;//Preemption Priority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; //Sub Priority
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	if( spiNum == 1 )
		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn; //TIM4 IRQ Channel
	else
		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn; //TIM4 IRQ Channel
	NVIC_Init(&NVIC_InitStructure);

	DMA_ITConfig(dma, DMA_IT_TC, ENABLE);
#endif
}

void SPIDriver::initDMA()
{
	DMA_InitTypeDef     DMA_InitStructure;

	DMA_DeInit(dma);
	// Configure SPI_BUS TX Channel
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // From memory to SPI
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&spi->DR;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = 0; // To be set later
	DMA_InitStructure.DMA_BufferSize = 1; // To be set later
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;

	DMA_Init(dma, &DMA_InitStructure);
}

uint32_t SPIDriver::requiredResources()
{
	if( spiNum == 1 )
		return RESOURCE_DMA_SPI_1;
	return RESOURCE_DMA_SPI_2;
}

void SPIDriver::transferDone()
{
	if( callback == 0 )
		return;
	ctx->done();
	DMA_Cmd(dma, DISABLE);

	TransferCompletedCb cb = callback;
	callback = 0;
	cb(callbackArg);
}

void SPIDriver::checkTransferCompleted()
{
	if( callback == 0 )
		return;

	if( spiNum == 1 )
	{
		if(DMA_GetFlagStatus(DMA1_FLAG_TC3) == RESET)
			return;
		if(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
			return;
		if(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
			return;

		DMA_ClearFlag(DMA1_FLAG_TC3);
		DMA_ClearFlag(DMA1_FLAG_GL3);
	}
	else if( spiNum == 2 )
	{
		if(DMA_GetFlagStatus(DMA1_FLAG_TC5) == RESET)
			return;
		if(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
			return;
		if(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET)
			return;

		DMA_ClearFlag(DMA1_FLAG_TC5);
		DMA_ClearFlag(DMA1_FLAG_GL5);
	}

	transferDone();
}


void SPIDriver::configureDMA(TransferType ttp)
{
	uint32_t tmpreg = dma->CCR;
	tmpreg &= ~(DMA_PeripheralDataSize_HalfWord | DMA_MemoryDataSize_HalfWord |
				DMA_PeripheralDataSize_Byte | DMA_PeripheralDataSize_Byte |
				DMA_MemoryInc_Disable | DMA_MemoryInc_Enable );
	if( ttp == TT_DATA_16_BIT || ttp == TT_FLOOD_16_BIT )
	{
		tmpreg |= DMA_PeripheralDataSize_HalfWord | DMA_MemoryDataSize_HalfWord;
		SPI_DataSizeConfig(spi, SPI_DataSize_16b);
	} else {
		tmpreg |= DMA_PeripheralDataSize_Byte | DMA_MemoryDataSize_Byte;
		SPI_DataSizeConfig(spi, SPI_DataSize_8b);
	}

	if( ttp == TT_FLOOD_8_BIT || ttp == TT_FLOOD_16_BIT || ttp == TT_FLOOD_32_BIT )
		tmpreg |= DMA_MemoryInc_Disable;
	else
		tmpreg |= DMA_MemoryInc_Enable;

	dma->CCR = tmpreg;
}

void SPIDriver::startDataTransfer(AbstractDriverContext * ctxIn, unsigned len, volatile const uint8_t * buffer, TransferType tt, TransferCompletedCb cb, void * cbArg)
{
	callback = cb;
	callbackArg = cbArg;
	ctx = ctxIn;
	ctx->start(tt);

	configureDMA(tt);

	dma->CNDTR = len;
	dma->CMAR = (uint32_t) buffer;

	int speed = ctx->getTransferSpeed();
	if( speed > -1 )
	{
		spi->CR1 &= ~(SPI_BaudRatePrescaler_2 | SPI_BaudRatePrescaler_4 | SPI_BaudRatePrescaler_8 |
					  SPI_BaudRatePrescaler_16 | SPI_BaudRatePrescaler_32 | SPI_BaudRatePrescaler_64 |
					  SPI_BaudRatePrescaler_128 | SPI_BaudRatePrescaler_256 );
		spi->CR1 |= speed;
	}

	DMA_Cmd(dma, ENABLE);
}

AbstractDriverContext * SPIDriver::createSPIContext(STM32Pin cs, int prescaler)
{
	class SPIDriverContext : virtual public AbstractDriverContext
	{
	private:
		AbstractDriver * driver;
		STM32Pin cs;
		int prescaler;

	public:
		SPIDriverContext(AbstractDriver * driverIn, STM32Pin csIn, int prescalerIn) : driver(driverIn), cs(csIn), prescaler(prescalerIn)
		{
			enableRCC(csIn);
			pinMode(csIn, GPIO_Mode_Out_PP, GPIO_Speed_50MHz);
			writePin(csIn, true);
		};

		void delay()
		{
			// slow down fast devices
			volatile int num = 1 << (prescaler / (SPI_BaudRatePrescaler_4 - SPI_BaudRatePrescaler_2));
			while(num--);
		}

		virtual int getTransferSpeed() { return prescaler; }

		virtual void start(TransferType /*type*/)
		{
			writePin(cs, false);
			delay();
		}

		virtual void done()
		{
			delay();
			writePin(cs, true);
		}

		virtual AbstractDriver * getDriver() { return driver; }

		virtual ~SPIDriverContext() {}
	};

	return new SPIDriverContext(this, cs, prescaler);
}

AbstractDriverContext * SPIDriver::createSPILCDContext(STM32Pin cs, STM32Pin a0, STM32Pin backlight, int prescaler)
{
	class SPILCDDriverContext : virtual public AbstractDriverContext
	{
	private:
		AbstractDriver * driver;
		STM32Pin cs;
		STM32Pin a0;
		int prescaler;

	public:
		SPILCDDriverContext(AbstractDriver * driverIn, STM32Pin csIn, STM32Pin a0In, STM32Pin backlight, int prescalerIn) : driver(driverIn), cs(csIn), a0(a0In), prescaler(prescalerIn)
		{
			enableRCC(csIn);
			pinMode(csIn, GPIO_Mode_Out_PP, GPIO_Speed_50MHz);
			writePin(csIn, true);

			enableRCC(a0In);
			pinMode(a0In, GPIO_Mode_Out_PP, GPIO_Speed_50MHz);
			writePin(a0In, false);

			enableRCC(backlight);
			pinMode(backlight, GPIO_Mode_Out_PP);
			writePin(backlight, true);
		};

		virtual void start(TransferType tt)
		{
			writePin(a0, tt != TT_COMMAND);
			writePin(cs, false);
		}

		virtual void done()
		{
			writePin(cs, true);
		}

		virtual int getTransferSpeed() { return prescaler; }

		virtual AbstractDriver * getDriver() { return driver; }

		virtual ~SPILCDDriverContext() {}
	};

	return new SPILCDDriverContext(this, cs, a0, backlight, prescaler);
}
