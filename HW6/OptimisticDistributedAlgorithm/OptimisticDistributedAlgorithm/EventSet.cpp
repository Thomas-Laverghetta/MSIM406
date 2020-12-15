#include "EventSet.h"

EventSet::Event::Event() {
    _ea = 0;
    _et = 0.0f;
    _next = 0;
    _prev = 0;
}

EventSet::Event::Event(const Time& t, EventAction* ea) {
    _ea = ea;
    _et = t;
    _next = 0;
    _prev = 0;
}

void EventSet::AddEvent(const Time& t, EventAction* ea) {

    // testing if ea is anti-msg (event class id == 0)
    if (ea->GetEventClassId() == ANTI_MSG) {
        if (_nextSchEvent && _nextSchEvent->_et < t) {
            Event* curr = _nextSchEvent;
            while (curr->_next && curr->_next->_et <= t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                curr = curr->_next;
            }
            // if curr exists (not null) and correct time, remove event from list
            if (curr->_next && curr->_next->_et == t) {
                Event* tmp = curr->_next;

                curr->_next = tmp->_next;

                if (tmp->_next) {
                    tmp->_next->_prev = curr;
                }

                // deleting event
                delete tmp->_ea;
                delete ea;
                delete tmp;
                tmp = 0;
            }
            // schedule anti-msg
            else {
                Event* A = new Event(t, ea);
                // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                A->_next = curr->_next;
                A->_prev = curr;
                curr->_next = A;

                if (A->_next)
                    A->_next->_prev = A;
            }
        }
        // search executed set
        else if (_prevExecEvent && _prevExecEvent->_et > t) {
            Event* curr = _prevExecEvent;
            while (curr->_prev && curr->_prev->_et >= t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t)) {
                curr = curr->_prev;
            }
            // if curr exists (not null) and correct time, remove event from list
            if (curr->_prev && curr->_prev->_et == t) {
                Event* tmp = curr->_prev;

                curr->_prev = tmp->_prev;

                if (tmp->_prev) {
                    tmp->_prev->_next = curr;
                }

                // number of rollbacks
                rollbacks++;

                // rolling back
                while (_prevExecEvent != tmp->_prev)
                {
                    _prevExecEvent->_ea->SendAntiMsg();
                    _prevExecEvent = _prevExecEvent->_prev;

                    // counting number of rolled events
                    numEventRolls++;
                }

                if (tmp->_prev)
                    _prevExecEvent = tmp->_prev->_prev;
                else
                    _prevExecEvent = 0;

                _nextSchEvent = tmp->_prev;

                // Send anti-msgs
                tmp->_ea->SendAntiMsg();

                // counting number of events rolled
                numEventRolls++;

                // deleting event
                delete tmp->_ea;
                delete tmp;
                tmp = 0;
                delete ea;
            }
            // schedule anti-msg
            else {
                Event* A = new Event(t, ea);
                // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                A->_prev = curr->_prev;
                A->_next = curr;
                curr->_prev = A;

                if (A->_prev)
                    A->_prev->_next = A;
            }
        }
        // if time equals either
        else if ((_nextSchEvent && _nextSchEvent->_et == t) || (_prevExecEvent && _prevExecEvent->_et == t)) {
            // checking head
            if (_nextSchEvent && _nextSchEvent->_ea->GetEventId() == ea->GetEventId()) {
                Event* tmp = _nextSchEvent;

                _nextSchEvent = _nextSchEvent->_next;
                if (_nextSchEvent)
                    _nextSchEvent->_prev = _prevExecEvent;

                if (_prevExecEvent)
                    _prevExecEvent->_next = _nextSchEvent;

                // deleting event
                delete tmp->_ea;
                delete ea;

                // deleting node
                delete tmp;
                tmp = 0;
            }
            else {
                // short circuiting for while and if statement if next-scheduled event does not exist
                Event* curr;
                if (_nextSchEvent)
                    curr = _nextSchEvent;   // if next scheduled event exists, then search scheduled list
                else
                    curr = _prevExecEvent;  // else (no next scheduled event), skip while and if below (curr->next == null since scheduled event is null)

                while (curr->_next && curr->_next->_et == t && curr->_next->_ea->GetEventId() != ea->GetEventId()) {
                    curr = curr->_next;
                }
                // if curr exists (not null) and correct time, remove event from list
                if (curr->_next && curr->_next->_et == t) {
                    Event* tmp = curr->_next;

                    curr->_next = tmp->_next;

                    if (tmp->_next) {
                        tmp->_next->_prev = curr;
                    }

                    // deleting event
                    delete tmp->_ea;
                    delete ea;
                    delete tmp;
                    tmp = 0;
                }
                else {
                    if (_prevExecEvent && _prevExecEvent->_ea->GetEventId() == ea->GetEventId() && _prevExecEvent->_et == t) {
                        Event* tmp = _prevExecEvent;

                        // increment number of rollbacks
                        rollbacks++;
                        numEventRolls++;

                        // rollback
                        if (_prevExecEvent->_prev) {
                            _prevExecEvent->_prev->_next = _nextSchEvent;
                            _prevExecEvent = _prevExecEvent->_prev->_prev;
                        }
                        else {
                            _prevExecEvent = 0;
                        }

                        if (_nextSchEvent)
                            _nextSchEvent->_prev = tmp->_prev;
                        _nextSchEvent = tmp->_prev;

                        // Send anti-msgs
                        tmp->_ea->SendAntiMsg();

                        // deleting event
                        delete tmp->_ea;
                        delete tmp;
                        tmp = 0;
                        delete ea;
                    }
                    else {
                        // short circuiting
                        if (_prevExecEvent)
                            curr = _prevExecEvent;  // if previous event exist, then search events previously executed
                        else
                            curr = _nextSchEvent;   // else (no previous events), then skip while and if (curr -> pre == null since previous event is null)

                        while (curr->_prev && curr->_prev->_et == t && curr->_prev->_ea->GetEventId() != ea->GetEventId()) {
                            curr = curr->_prev;
                        }
                        // if curr exists (not null) and correct time, remove event from list
                        if (curr->_prev && curr->_prev->_et == t) {
                            Event* tmp = curr->_prev;

                            curr->_prev = tmp->_prev;

                            if (tmp->_prev) {
                                tmp->_prev->_next = curr;
                            }

                            // incrment number of rollbacks
                            rollbacks++;

                            // rolling back
                            while (_prevExecEvent != tmp->_prev)
                            {
                                _prevExecEvent->_ea->SendAntiMsg();
                                _prevExecEvent = _prevExecEvent->_prev;

                                // counting number of events rolled
                                numEventRolls++;
                            }

                            if (tmp->_prev)
                                _prevExecEvent = tmp->_prev->_prev;
                            else
                                _prevExecEvent = 0;

                            _nextSchEvent = tmp->_prev;

                            // Send anti-msgs
                            tmp->_ea->SendAntiMsg();

                            // counting number of events rolled
                            numEventRolls++;

                            // deleting event
                            delete tmp->_ea;
                            delete tmp;
                            tmp = 0;
                            delete ea;
                        }
                        // schedule anti-msg between _prevExecEvent and _nextSchEvent
                        else {
                            Event* A = new Event(t, ea);
                            A->_next = _nextSchEvent;
                            A->_prev = _prevExecEvent;

                            if (_nextSchEvent)
                                _nextSchEvent->_prev = A;
                            if (_prevExecEvent)
                                _prevExecEvent->_next = A;

                            _prevExecEvent = A;
                        }
                    }
                }
            }
        }
        // if _prevExecEvent is null or _prevExecEvent->t < t < _nextSchEvent
        else {
            Event* A = new Event(t, ea);
            A->_next = _nextSchEvent;
            A->_prev = _prevExecEvent;

            if (_nextSchEvent)
                _nextSchEvent->_prev = A;
            if (_prevExecEvent)
                _prevExecEvent->_next = A;

            _prevExecEvent = A;
        }
    }
    // not a anti-msg - real event
    else {
        // if time is less than last executed time
        if (_prevExecEvent && _prevExecEvent->_et > t) {
            // Locate the node before the point of insertion 
            Event* curr = _prevExecEvent;
            // while previous node is exist and less than t and not equal too anti-msg on that node
            while (curr->_prev &&
                curr->_prev->_et >= t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t))
            {
                curr = curr->_prev;
            }
            // if not anti-msg, schedule event
            if (!(curr->_prev && curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t)) {
                Event* new_event = new Event(t, ea);
                new_event->_next = curr;
                new_event->_prev = curr->_prev;
                curr->_prev = new_event;

                if (new_event->_prev) {
                    new_event->_prev->_next = new_event;
                }

                // rollback
                while (_prevExecEvent != new_event) {
                    // sending anti-msgs
                    _prevExecEvent->_ea->SendAntiMsg();
                    _prevExecEvent = _prevExecEvent->_prev;

                    // counting number of events rolled
                    numEventRolls++;
                }

                // counting number of rollbacks
                rollbacks++;

                // setting executed 
                _prevExecEvent = new_event->_prev;

                // setting curr to new node
                _nextSchEvent = new_event;

            }
            // anti-msg found, remove anti-msg and delete ea
            else {
                // remove anti-msg and do not schedule new msg
                Event* anti = curr->_prev;
                curr->_prev = anti->_prev;
                if (anti->_prev)
                    anti->_prev->_next = curr;

                // delete event
                delete ea;
                delete anti->_ea;
                delete anti;
                anti = 0;
            }
        }
        // if time is greater than next sim-time
        else if (_nextSchEvent && _nextSchEvent->_et < t) {
            // Locate the node before the point of insertion 
            Event* curr = _nextSchEvent;
            while (curr->_next &&
                curr->_next->_et <= t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t))
            {
                curr = curr->_next;
            }
            if (!(curr->_next && curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                Event* new_event = new Event(t, ea);
                new_event->_next = curr->_next;
                new_event->_prev = curr;

                if (curr->_next)
                    curr->_next->_prev = new_event;

                curr->_next = new_event;
            }
            // anti-msg found
            else {
                // remove anti-msg and do not schedule new msg
                Event* anti = curr->_next;
                curr->_next = anti->_next;
                if (anti->_next)
                    anti->_next->_prev = curr;

                // delete ea
                delete ea;
                delete anti->_ea;
                delete anti;
                anti = 0;
            }
        }
        // if time equals either
        else if ((_nextSchEvent && _nextSchEvent->_et == t) || (_prevExecEvent && _prevExecEvent->_et == t)) {
            if (_prevExecEvent && _prevExecEvent->_et == t && _prevExecEvent->_ea->GetEventId() == ea->GetEventId()) {
                // remove anti-msg and do not schedule new msg
                Event* anti = _prevExecEvent;
                _prevExecEvent = anti->_prev;
                if (_prevExecEvent)
                    _prevExecEvent->_next = _nextSchEvent;

                if (_nextSchEvent)
                    _nextSchEvent->_prev = _prevExecEvent;

                // delete event
                delete ea;
                delete anti->_ea;
                delete anti;
                anti = 0;
            }
            else {
                // searching for anti-msgs in executed set
                Event* curr;
                if (_prevExecEvent)
                    curr = _prevExecEvent;
                else
                    curr = _nextSchEvent;

                // while a previous event exists w/same timestamp, check for anti-msgs
                while (curr->_prev &&
                    curr->_prev->_et == t && curr->_prev->_ea->GetEventId() != ea->GetEventId())
                {
                    curr = curr->_prev;
                }
                // if anti-msg was found
                if (curr->_prev && curr->_prev->_et == t) {
                    // remove anti-msg and do not schedule new msg
                    Event* anti = curr->_prev;
                    curr->_prev = anti->_prev;
                    if (anti->_prev)
                        anti->_prev->_next = curr;

                    // delete event
                    delete ea;
                    delete anti->_ea;
                    delete anti;
                    anti = 0;
                }
                // search scheduled events
                else {
                    // short circuit
                    if (_nextSchEvent)
                        curr = _nextSchEvent;
                    else
                        curr = _prevExecEvent;

                    // while next event w/same timestamp, check for anti-msgs
                    while (curr->_next &&
                        curr->_next->_et == t && curr->_next->_ea->GetEventId() != ea->GetEventId())
                    {
                        curr = curr->_next;
                    }
                    // anti-msg found
                    if (curr->_next && curr->_next->_et == t) {
                        // remove anti-msg and do not schedule new msg
                        Event* anti = curr->_next;
                        curr->_next = anti->_next;
                        if (anti->_next)
                            anti->_next->_prev = curr;

                        // delete ea
                        delete ea;
                        delete anti->_ea;
                        delete anti;
                        anti = 0;
                    }
                    // anti-msg not found, sch anti-msg
                    else {
                        Event* new_event = new Event(t, ea);
                        new_event->_next = _nextSchEvent;
                        new_event->_prev = _prevExecEvent;

                        if (_nextSchEvent)
                            _nextSchEvent->_prev = new_event;
                        if (_prevExecEvent)
                            _prevExecEvent->_next = new_event;

                        _nextSchEvent = new_event;
                    }
                }
            }
        }
        // if _nextSchEvent is either null or _prevExecEvent->t < t < _nextSchEvent->t
        else {
            Event* new_event = new Event(t, ea);
            new_event->_next = _nextSchEvent;
            new_event->_prev = _prevExecEvent;

            if (_nextSchEvent)
                _nextSchEvent->_prev = new_event;
            if (_prevExecEvent)
                _prevExecEvent->_next = new_event;

            _nextSchEvent = new_event;
        }
    }
}

EventAction* EventSet::GetEventAction() {
    // move exec to next event
    _prevExecEvent = _nextSchEvent;

    // move curr to next event 
    _nextSchEvent = _nextSchEvent->_next;

    // execute next event
    return _prevExecEvent->_ea;
}

Time EventSet::GetEventTime() {
    return _nextSchEvent->_et;
}

void EventSet::GVT_removal(const Time& GVT)
{
    Event* curr = _prevExecEvent;
    
    // iterating curr event time is less than or equal to GVT
    while (curr && curr->_et >= GVT)
        curr = curr->_prev;
    
    if (curr && curr->_next)
        curr->_next->_prev = 0;

    // while curr exists, remove all previous events
    while (curr) {
        Event* tmp = curr;
        
        // move onto the next 
        curr = curr->_prev;
    
        // removing node
        delete tmp;
        tmp = 0;
    }
}