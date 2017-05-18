#ifndef RESOURCE_H_
#define RESOURCE_H_

#define RESOURCE_ADC_1      (1 << 0)
#define RESOURCE_ADC_2      (1 << 1)
#define RESOURCE_ADC_3      (1 << 2)
#define RESOURCE_SPI_1      (1 << 3)
#define RESOURCE_SPI_2      (1 << 4)
#define RESOURCE_DMA1_CH_1  (1 << 5)
#define RESOURCE_DMA1_CH_3  (1 << 6)
#define RESOURCE_DMA1_CH_5  (1 << 7)

#define RESOURCE_DMA_SPI_1 ( RESOURCE_DMA1_CH_3 + RESOURCE_SPI_1 )
#define RESOURCE_DMA_SPI_2 ( RESOURCE_DMA1_CH_5 + RESOURCE_SPI_2 )
#define RESOURCE_DMA_ADC_1 ( RESOURCE_DMA1_CH_1 + RESOURCE_ADC_1 )

#endif /* RESOURCE_H_ */
