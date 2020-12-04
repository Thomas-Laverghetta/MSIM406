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
    unsigned int rollbacks; //  Number of rollbacks occured
    unsigned int numEventRolls;// number of event that rolled backed
public:
    // saves event
    void AddEvent(const Time& t, EventAction * ea);

    // Returns size of Event Set
    bool isEmpty() {
        // skip anti-msgs
        Node* tmp = _curr;
        while (tmp && tmp->_ea->GetEventClassId() == ANTI_MSG) {
            tmp = tmp->_next;
        }
        // if tmp exists, then no anti-msg at location (found executable event)
        if (tmp) {
            // move curr to tmp and move exec to pre
            _curr = tmp;
            _exec = tmp->_prev;
            return true;
        }
        else
            return false;
    }

    // returns event with smallest time 
    EventAction * GetEventAction();

    // returns time with smallest time stamp and deletes event from set
    Time GetEventTime();

    // Defualt constructor
    inline EventSet(){
        rollbacks = 0;
        numEventRolls = 0;
        _curr = 0;
        _exec = 0;
    }

    ~EventSet() {
        Node* tmp;
        while (_exec) {
            tmp = _exec;
            _exec = _exec->_prev;
            delete tmp->_ea;
            delete tmp;
            tmp = 0;
        }
        while (_curr) {
            tmp = _curr;
            _curr = _curr->_next;
            delete tmp->_ea;
            delete tmp;
            tmp = 0;
        }

        printf("PROC=%i | Avg. Num Events Rolled=%f | Num Rolls=%i | Num Events Rolled=%i\n", CommunicationRank(), (float)numEventRolls / ((float)rollbacks), rollbacks, numEventRolls); fflush(stdout);
    }
};
#endif