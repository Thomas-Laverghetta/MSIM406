#ifndef EVENTSET_H
#define EVENTSET_H

#include "SimulationExecutive.h"

/*
    This implementation will use a sorted linked list
*/
class EventSet
{
private:
    // Event Set data
    class Node{
    public:
        Node * _next;
        Node*   _prev;
        EventAction * _ea;
        Time _et;

        Node();
        Node(const Time& t, EventAction * ea);
    };
  
    Node* _curr;            // smallest timestamped scheduled event
    Node* _exec;            // last event executed
public:
    // saves event
    void AddEvent(const Time& t, EventAction * ea);

    // Returns size of Event Set
    bool isEmpty() {
        // skip anti-msgs
        if (_curr && _curr->_ea->GetEventClassId() == 0)
            _curr = _curr->_next;
        return !_curr;
    }

    // returns event with smallest time 
    EventAction * GetEventAction();

    // returns time with smallest time stamp and deletes event from set
    Time GetEventTime();

    // Defualt constructor
    inline EventSet(){
        _curr = 0;
        _exec = 0;
    }
};
#endif