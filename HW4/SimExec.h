#ifndef SIMULATIONEXECUTIVE_H
#define SIMULATIONEXECUTIVE_H

#include "Event.h"

typedef double Time;

class SimulationExecutive
{
public:
	static SimulationExecutive *Instance();
	static void RunSimulation( Time endTime);
private:
	void ScheduleEvent( Time time, Event *ev);
	Time GetCurrentTime();
	SimulationExecutive();
	static SimulationExecutive *_instance;
	class EventList;
	EventList *_eventList;
	Time _currentTime;
};

#endif
