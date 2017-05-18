#ifndef TRIGGERWIDGET_H_
#define TRIGGERWIDGET_H_

#include "graphics/TaskCanvas.h"
#include "MeasurementData.h"

class TriggerWidget
{
private:
	MeasurementData * data;

public:
	TriggerWidget(MeasurementData *dataIn);

	void draw(TaskCanvas *canvas);
};

extern const uint8_t bmp_trigger_none [];
extern const uint8_t bmp_trigger_rising_edge [];
extern const uint8_t bmp_trigger_falling_edge [];
extern const uint8_t bmp_trigger_changing_edge [];
extern const uint8_t bmp_trigger_positive_peak [];
extern const uint8_t bmp_trigger_negative_peak [];
extern const uint8_t bmp_trigger_external [];
extern const uint8_t bmp_trigger_none_fft [];

#endif /* TRIGGERWIDGET_H_ */
