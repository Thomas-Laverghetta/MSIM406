#ifndef EVENTSET_H
#define EVENTSET_H

#include "SimulationExecutive.h"
#define ANTI_MSG 0
/*
    This implementation will use a sorted linked list
*/
class EventSet
{
private:
    // Event Set data
    class Event{
    public:
        Event * _next;
        Event*   _prev;
        EventAction * _ea;
        Time _et;

        Event();
        Event(const Time& t, EventAction * ea);
    };
  
    Event* _nextSchEvent;            // smallest timestamped scheduled event
    Event* _prevExecEvent;            // last event executed
    unsigned int rollbacks; //  Number of rollbacks occured
    unsigned int numEventRolls;// number of event that rolled backed
public:
    // saves event
    void AddEvent(const Time& t, EventAction * ea);

    // Returns size of Event Set
    bool isEmpty() {
        // skip anti-msgs
        Event* tmp = _nextSchEvent;
        while (tmp && tmp->_ea->GetEventClassId() == ANTI_MSG) {
            tmp = tmp->_next;
        }
        // if tmp exists, then no anti-msg at location (found executable event)
        if (tmp) {
            // move curr to tmp and move exec to pre
            _nextSchEvent = tmp;
            _prevExecEvent = tmp->_prev;
            return false;
        }
        else
            return true;
    }

    // returns event with smallest time 
    EventAction * GetEventAction();

    // returns time with smallest time stamp and deletes event from set
    Time GetEventTime();

    // Defualt constructor
    inline EventSet(){
        rollbacks = 0;
        numEventRolls = 0;
        _nextSchEvent = 0;
        _prevExecEvent = 0;
    }

    ~EventSet() {
        Event* tmp;
        while (_prevExecEvent) {
            tmp = _prevExecEvent;
            _prevExecEvent = _prevExecEvent->_prev;
            delete tmp->_ea;
            delete tmp;
            tmp = 0;
        }
        while (_nextSchEvent) {
            tmp = _nextSchEvent;
            _nextSchEvent = _nextSchEvent->_next;
            delete tmp->_ea;
            delete tmp;
            tmp = 0;
        }

        printf("PROC=%i | Avg. Num Events Rolled=%f | Num Rolls=%i | Num Events Rolled=%i\n", CommunicationRank(), (float)numEventRolls / ((float)rollbacks), rollbacks, numEventRolls); fflush(stdout);
    }
};
#endif