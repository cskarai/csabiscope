#ifndef OSCILLOSCOPESCREEN_H_
#define OSCILLOSCOPESCREEN_H_

#include "graphics/TaskCanvas.h"
#include "MeasurementData.h"

class OscilloscopeScreen
{
private:
	MeasurementData * data;

public:
	OscilloscopeScreen(MeasurementData *data);

	void draw(TaskCanvas *canvas);
};

#endif /* OSCILLOSCOPESCREEN_H_ */
