#include "Config.h"
#include <stm32f10x_crc.h>
#include <stm32f10x_flash.h>
#include <string.h>

#define CONFIG_ADDRESS 0x0801FC00
#define PAGE_SIZE      0x400

Config config;

void Config::calculateCRC()
{
	CRC_ResetDR();
	crc = 0;

	CRC_CalcBlockCRC((uint32_t *)this, sizeof(Config)/sizeof(uint32_t));
	crc = CRC_GetCRC();
}

void Config::loadConfig(Config * loadConfig)
{
	memcpy(loadConfig, (void *)CONFIG_ADDRESS, sizeof(Config));
	uint32_t crc = loadConfig->crc;
	loadConfig->calculateCRC();

	// the calculated CRC is invalid
	if( crc != loadConfig->crc )
	{
		Config init;
		memcpy(loadConfig, &init, sizeof(Config));
	}
}

void Config::saveConfig(Config * saveConfig)
{
	uint8_t buf[PAGE_SIZE];
	memset(buf, 0xFF, PAGE_SIZE);
	saveConfig->calculateCRC();
	memcpy(buf, saveConfig, sizeof(Config));

	FLASH_Unlock();
	FLASH_ErasePage( CONFIG_ADDRESS );

	for(unsigned i=0; i < sizeof(buf); i+=2)
	{
		uint16_t hword = buf[i];
		hword |= (uint16_t)(buf[i+1]) << 8;

		FLASH_ProgramHalfWord(CONFIG_ADDRESS+i, hword);
	}

	FLASH_Lock();

}

void Config::init()
{
	RCC_AHBPeriphClockCmd(RCC_AHBENR_CRCEN, ENABLE);
	Config::loadConfig(&config);
}
