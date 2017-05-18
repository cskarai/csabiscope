#ifndef TASK_TASKHANDLER_H_
#define TASK_TASKHANDLER_H_

#include <list>

#include <inttypes.h>
#include <task/AbstractTask.h>

class TaskHandler
{
private:
	static TaskHandler *      instance;
	uint32_t                  availableResources;
	std::list<AbstractTask *> runningTaskList;
	std::list<AbstractTask *> waitingTaskList;

public:
	TaskHandler();

	void scheduleTask(AbstractTask *task, int32_t shot = 0, int32_t repeat = -1);
	int  getTasksToProcess(uint32_t resource);

	void loop();

	static TaskHandler & getInstance() { return *instance; }
};

#endif /* TASK_TASKHANDLER_H_ */
