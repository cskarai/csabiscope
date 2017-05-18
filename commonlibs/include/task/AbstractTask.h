#ifndef ABSTRACTTASK_H_
#define ABSTRACTTASK_H_

#define TASK_SINGLE_SHOT    (-1)

#include <inttypes.h>

typedef enum
{
	TS_WAITING,
	TS_STARTED,
	TS_DONE,
	TS_RESUBMITTED,
} TaskState;

class TaskHandler;

class AbstractTask
{
	friend class TaskHandler;

private:
	uint32_t      nextRun;
	int32_t       repeatInterval = -1;
	TaskState     state = TS_WAITING;
	bool          autoDelete = true;
	bool          cacheBuilt = false;

public:
	virtual uint32_t requiredResources() {return 0;}
	virtual void     start() = 0;
	virtual void     loop() {};

	virtual void     buildCachedData() {};
	virtual bool     hasCachedData() { return false; }
	void             invalidateCache() { cacheBuilt = false; }
	void             buildCache() { if( hasCachedData() && !cacheBuilt ) { buildCachedData(); cacheBuilt = true;} };

	int32_t          getRepeatInterval() { return repeatInterval; }
	void             setRepeatInterval(int32_t repeat) { repeatInterval = repeat; }
	bool             isAutoDelete() { return autoDelete; }
	void             setAutoDelete(bool del) { autoDelete = del; }
	TaskState        getState() { return state; }
	void             setState(TaskState st) {state = st;}
	void             taskCompleted() { setState(TS_DONE); }
	void             resubmitTask() { setState(TS_RESUBMITTED); }

	virtual ~AbstractTask() {}
};

#endif /* ABSTRACTTASK_H_ */

