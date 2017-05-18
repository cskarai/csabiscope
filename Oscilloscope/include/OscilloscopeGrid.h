/*
 * OscilloscopeGrid.h
 *
 *  Created on: 2016. dec. 31.
 *      Author: ckarai
 */

#ifndef OSCILLOSCOPEGRID_H_
#define OSCILLOSCOPEGRID_H_

#define GRID_X                 8
#define GRID_Y                17

#include "task/AbstractTask.h"
#include "graphics/TaskCanvas.h"
#include "MeasurementData.h"

class OscilloscopeGrid : public virtual AbstractTask
{
private:
	TaskCanvas * canvas;
	MeasurementData * data;
	unsigned x;
	unsigned y;
	unsigned currentX;

	void renderScopeGrid(unsigned currentX);
	void renderChannel(unsigned channel, uint8_t * colors, unsigned currentX, unsigned segmSize);

public:
	OscilloscopeGrid(TaskCanvas * canvasIn, MeasurementData *dataIn, unsigned xIn, unsigned yIn);

	virtual void start();

	virtual bool hasCachedData() { return true; }
	virtual void buildCachedData();

	virtual uint32_t requiredResources() {return canvas->requiredResources();}
};



#endif /* OSCILLOSCOPEGRID_H_ */
