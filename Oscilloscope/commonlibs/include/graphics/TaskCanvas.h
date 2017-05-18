#ifndef TASKCANVAS_H_
#define TASKCANVAS_H_

#include "driver/AbstractDriver.h"
#include "task/AbstractTask.h"
#include "graphics/Canvas.h"
#include "task/TaskHandler.h"

class ScenarioPlayerTask;

class TaskCanvas : public virtual Canvas
{
	friend class ScenarioPlayerTask;

private:
	static void processBitmapSegment(TaskCanvas * canvas, void * args);
	static void processTextSegment(TaskCanvas * canvas, void * args);
	static void processButton(TaskCanvas * canvas, void * args);

public:
	virtual ColorBuffer * allocateColorBuffer(unsigned x, unsigned y, unsigned w, unsigned h, Angle angle = ANGLE_0) override;
	virtual void      fillColors(ColorBuffer * buffer) override;
	virtual void      floodColor(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color) override;
	virtual void      drawBitmapSegments(BitmapSegmenterDescriptor * desc) override;
	virtual void      drawTextSegments(TextSegmenterDescriptor * desc) override;
	virtual void      drawButton(ButtonDescriptor *) override;

	virtual AbstractDriverContext * getDriverContext() = 0;
	void              playScenario(ScenarioPlayerTask * scpTask);

	uint32_t          requiredResources();

	virtual unsigned  addWindowAddrScenario(uint8_t * dest, unsigned x, unsigned y, unsigned w, unsigned h, Angle angle = ANGLE_0) = 0;

protected:
	~TaskCanvas() {}
};

typedef void (*ActionCallback)(TaskCanvas *canvas, void *args);

class ScheduledAction : public virtual AbstractTask
{
private:
	TaskCanvas     * canvas;
	ActionCallback   callback;
	void           * args;

public:
	ScheduledAction(TaskCanvas *canvasIn, ActionCallback callbackIn, void * argsIn = 0) :
		canvas(canvasIn), callback(callbackIn), args(argsIn)
	{
		TaskHandler::getInstance().scheduleTask(this);
	}

	virtual bool hasCachedData() { return true; }

	virtual uint32_t requiredResources() {return canvas->requiredResources();}

	virtual void buildCachedData()
	{
		if( callback != 0 )
			callback(canvas, args);
		callback = 0;
	}

	virtual void start()
	{
		taskCompleted();
	}
};

template<class W> void scheduleWidget(TaskCanvas * canvas, W * widget)
{
	new ScheduledAction(canvas, [] (TaskCanvas * canvas, void * arg) {
		((W *)arg)->draw(canvas);
		delete (W*)arg;
	}, widget);
}

#include "graphics/ScenarioPlayerTask.h"

#endif /* TASKCANVAS_H_ */
