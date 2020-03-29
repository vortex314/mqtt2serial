#include "NanoAkka.h"

NanoStats stats;
/*
 _____ _                        _
|_   _| |__  _ __ ___  __ _  __| |
  | | | '_ \| '__/ _ \/ _` |/ _` |
  | | | | | | | |  __/ (_| | (_| |
  |_| |_| |_|_|  \___|\__,_|\__,_|
*/
int Thread::_id = 0;

void Thread::createQueue()
{
}

void Thread::start()
{
}

int Thread::enqueue(Invoker *invoker)
{
	//	INFO("Thread '%s' >>> '%s'",_name.c_str(),symbols(invoker));
	_workQueue.push(invoker);
	return 0;
};

void Thread::run()
{
	INFO("Thread '%s' started ", _name.c_str());
	while (true)
		loop();
}

void Thread::loop()
{
	uint64_t now = Sys::millis();
	uint64_t expTime = now + 5000;
	TimerSource *expiredTimer = 0;
	// find next expired timer if any within 5 sec
	for (auto timer : _timers)
	{
		if (timer->expireTime() < expTime)
		{
			expTime = timer->expireTime();
			expiredTimer = timer;
		}
	}
	int32_t waitTime = (expTime - now); // ESP_OPEN_RTOS seems to double sleep time ?

	//		INFO(" waitTime : %d ",waitTime);

	if (waitTime > 0)
	{
		Invoker *prq;
		if (_workQueue.pop(prq) == 0)
		{
			uint64_t start = Sys::millis();
			prq->invoke();
			uint32_t delta = Sys::millis() - start;
			if (delta > 20)
				WARN("Invoker [%X] slow %d msec invoker on thread '%s'.", prq, delta, _name.c_str());
		}
	}
	else
	{
		if (expiredTimer)
		{
			if (-waitTime > 100)
				INFO("Timer[%X] already expired by %u msec on thread '%s'.", expiredTimer, -waitTime, _name.c_str());
			uint64_t start = Sys::millis();
			expiredTimer->request();
			uint32_t deltaExec = Sys::millis() - start;
			if (deltaExec > 20)
				WARN("Timer [%X] request slow %d msec on thread '%s'", expiredTimer, deltaExec, _name.c_str());
		}
	}
}
