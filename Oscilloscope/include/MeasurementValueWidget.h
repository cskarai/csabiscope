#ifndef MEASUREMENTVALUEWIDGET_H_
#define MEASUREMENTVALUEWIDGET_H_

#include "graphics/TaskCanvas.h"
#include "MeasurementData.h"

class MeasurementValueWidget
{
private:
	MeasurementData * data;

public:
	MeasurementValueWidget(MeasurementData *dataIn);

	void draw(TaskCanvas *canvas);
	void channel(char * buffer, int channel);
};

#endif /* MEASUREMENTVALUEWIDGET_H_ */
