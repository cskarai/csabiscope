#include "graphics/ScenarioPlayerTask.h"
#include "systick/SysTime.h"
#include <stdlib.h>
#include <memory>

typedef enum
{
	SPST_DONE,
	SPST_WAIT_COMMAND_DONE,
	SPST_WAIT_DATA_DONE,
	SPST_WAIT_DELAY,
} ScPlayerState;

ScenarioPlayerTask::ScenarioPlayerTask(unsigned colorSizeIn, AbstractDriverContext *driverContextIn, const uint8_t * scenarioIn) :
				driverContext(driverContextIn), scenario(scenarioIn), state(SPST_DONE), colorSize(colorSizeIn)
{
}

ScenarioPlayerTask * ScenarioPlayerTask::createScenarioPlayer( unsigned colorSize, AbstractDriverContext *driverContext, const uint8_t * scenarioIn )
{
	return new ScenarioPlayerTask( colorSize, driverContext, scenarioIn );
}

ScenarioPlayerTask * ScenarioPlayerTask::createScenarioPlayer( unsigned colorSize, AbstractDriverContext *driverContext, unsigned len )
{
	uint8_t * alloc = new uint8_t [ sizeof(ScenarioPlayerTask) + len ];
	ScenarioPlayerTask * task = new (alloc) ScenarioPlayerTask(colorSize, driverContext, (const uint8_t *)(alloc + sizeof(ScenarioPlayerTask)));
	return task;
}

void ScenarioPlayerTask::transferCompleteCb(void *arg)
{
	((ScenarioPlayerTask *)arg)->transferCompleted();
}

void ScenarioPlayerTask::transferCompleted()
{
	switch( state )
	{
	case SPST_WAIT_DATA_DONE:
		jumpToNextCommand();
		break;
	case SPST_WAIT_COMMAND_DONE:
		{
			uint8_t len = *scenario & 127;
			if( len == 0 )
				jumpToNextCommand();
			else
			{
				state = SPST_WAIT_DATA_DONE;
				driverContext->getDriver()->startDataTransfer(driverContext, len, scenario+2, TT_DATA_8_BIT, ScenarioPlayerTask::transferCompleteCb, this);
			}
		}
		break;
	}
}

void ScenarioPlayerTask::jumpToNextCommand()
{
	state = SPST_DONE;

	if( *scenario < 0x80 ) // SC_COMMAND
		scenario += 2 + *scenario;
	else if (*scenario == SC_DELAY )
		scenario += 2;
	else if (*scenario == SC_FLOOD )
		scenario += 7;
	else if (*scenario == SC_DONE )
	{
		/* do nothing */
	}
	else if (*scenario == SC_SEND_BUFFER )
	{
		unsigned cnt = (scenario[1] + (scenario[2] << 8)) * colorSize;
		scenario += 3 + cnt;
	}
	playCommand();
}

void ScenarioPlayerTask::playCommand()
{
	while( *scenario == SC_PAD ) // padding bytes for 4 byte alignment
		scenario++;

	switch( *scenario )
	{
	case SC_DONE:
		taskCompleted();
		state = SPST_DONE;
		return;
	case SC_DELAY:
	{
		unsigned time = scenario[1];
		if( time == 255 )
			time = 500;

		data = (uint32_t)(SysTime::getMillis() + time);
		state = SPST_WAIT_DELAY;
		break;
	}
	case SC_SEND_BUFFER:
	{
		unsigned len = scenario[1] + 256*scenario[2];
		TransferType ttp;

		switch(colorSize)
		{
		case 2:
			ttp = TT_DATA_16_BIT;
			break;
		case 4:
			ttp = TT_DATA_32_BIT;
			break;
		case 1:
		default:
			ttp = TT_DATA_8_BIT;
			break;
		}

		state = SPST_WAIT_DATA_DONE;
		driverContext->getDriver()->startDataTransfer(driverContext, len, scenario+3, ttp, ScenarioPlayerTask::transferCompleteCb, this);
		break;
	}
	case SC_FLOOD:
	{
		unsigned len = scenario[1] + 256*scenario[2];
		TransferType ttp;

		switch(colorSize)
		{
		case 2:
			ttp = TT_FLOOD_16_BIT;
			break;
		case 4:
			ttp = TT_FLOOD_32_BIT;
			break;
		case 1:
		default:
			ttp = TT_FLOOD_8_BIT;
			break;
		}

		state = SPST_WAIT_DATA_DONE;
		driverContext->getDriver()->startDataTransfer(driverContext, len, scenario+3, ttp, ScenarioPlayerTask::transferCompleteCb, this);
		break;
	}
	default:
		if( *scenario < 0x80 )
		{
			state = SPST_WAIT_COMMAND_DONE;
			driverContext->getDriver()->startDataTransfer(driverContext, 1, scenario+1, TT_COMMAND, ScenarioPlayerTask::transferCompleteCb, this);
			break;
		}
	}
}

void ScenarioPlayerTask::start()
{
	playCommand();
}

void ScenarioPlayerTask::loop()
{
	switch( state )
	{
	case SPST_DONE:
		break;
	case SPST_WAIT_COMMAND_DONE:
	case SPST_WAIT_DATA_DONE:
		driverContext->getDriver()->checkTransferCompleted();
		break;
	case SPST_WAIT_DELAY:
	{
		uint64_t ts = SysTime::getMillis();
		uint64_t waitEnd = ( ts & 0xFFFFFFFF00000000 ) | data;
		if(( waitEnd > ts ) && ( waitEnd - ts > 0x7FFFFFFF ) )
			waitEnd -= 0x100000000;
		if(( waitEnd < ts ) && ( ts - waitEnd > 0x7FFFFFFF ) )
			waitEnd += 0x100000000;

		if( ts >= waitEnd )
			jumpToNextCommand();
	}
	break;
	}
}
