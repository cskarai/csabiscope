#include "task/TaskHandler.h"
#include "systick/SysTime.h"

#define MAX_CACHE_EXTRACTED 1

TaskHandler * TaskHandler::instance = 0;

TaskHandler::TaskHandler(): availableResources(0xFFFFFFFF)
{
	TaskHandler::instance = this;
}

void TaskHandler::scheduleTask(AbstractTask *task, int32_t nextShot, int32_t repeat)
{
	task->state = TS_WAITING;
	task->nextRun = (uint32_t)(SysTime::getMillis() + nextShot);
	task->repeatInterval = repeat;
	waitingTaskList.push_back(task);
}

void TaskHandler::loop()
{
	// loop over tasks
	std::list<AbstractTask *>::iterator iterator;
	for (iterator = runningTaskList.begin(); iterator != runningTaskList.end();) {
		AbstractTask * task = *iterator;
		task->loop();

		if( task->getState() == TS_DONE || task->getState() == TS_RESUBMITTED )
		{
			iterator = runningTaskList.erase(iterator);
			availableResources |= task->requiredResources();

			if( task->getState() == TS_RESUBMITTED )
				scheduleTask(task, 0, task->repeatInterval);
			else if( task->getRepeatInterval() == TASK_SINGLE_SHOT )
			{
				if( task->isAutoDelete() )
					delete task;
			}
			else
				scheduleTask(task, task->repeatInterval, task->repeatInterval);

			continue;
		}

		iterator++;
	}

	int cacheExtracted = 0;
	uint64_t ts = SysTime::getMillis();
	// start waiting tasks
	for (iterator = waitingTaskList.begin(); iterator != waitingTaskList.end();) {
		AbstractTask * task = *iterator;

		// build only one into cache
		if( task->hasCachedData() )
		{
			if( cacheExtracted < MAX_CACHE_EXTRACTED )
			{
				task->buildCache();
				cacheExtracted++;
			}
		}

		if(( task->requiredResources() & availableResources ) == task->requiredResources() )
		{
			uint64_t runTime = ( ts & 0xFFFFFFFF00000000 ) | task->nextRun;
			if(( runTime > ts ) && ( runTime - ts > 0x7FFFFFFF ) )
				runTime -= 0x100000000;
			if(( runTime < ts ) && ( ts - runTime > 0x7FFFFFFF ) )
				runTime += 0x100000000;

			if( runTime <= ts )
			{
				// we have all the resources to execute the task
				availableResources &= ~task->requiredResources();

				iterator = waitingTaskList.erase(iterator);
				runningTaskList.push_back(task);
				task->buildCache();
				task->start();
				continue;
			}
		}
		iterator++;
	}
}

int TaskHandler::getTasksToProcess(uint32_t resource)
{
	int cnt = 0;

	for (AbstractTask * task : runningTaskList) {
		if(( task->requiredResources() & resource ) == resource)
			cnt++;
	}

	for (AbstractTask * task : waitingTaskList) {
		if(( task->requiredResources() & resource ) == resource)
			cnt++;
	}

	return cnt;
}
