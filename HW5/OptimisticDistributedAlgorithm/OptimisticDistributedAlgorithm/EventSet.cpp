#include "EventSet.h"

EventSet::Node::Node(){
    _ea = 0;
    _et = 0.0f;    
    _next = 0;
    _prev = 0;
}

EventSet::Node::Node(const Time& t, EventAction * ea){
    _ea = ea;
    _et = t;
    _next = 0;
    _prev = 0;
}

void EventSet::AddEvent(const Time& t, EventAction * ea){
    
    // testing if ea is anti-msg (event class id == 0)
    if (ea->GetEventClassId() == ANTI_MSG) {
        if (_curr && _curr->_et < t) {
            Node * curr = _curr;
            while (curr->_next && curr->_next->_et <= t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                curr = curr->_next;
            }
            // if curr exists (not null) and correct time, remove event from list
            if (curr->_next && curr->_next->_et == t) {
                Node* tmp = curr->_next;

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
                Node* A = new Node(t, ea);
                // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                A->_next = curr->_next;
                A->_prev = curr;
                curr->_next = A;

                if (A->_next)
                    A->_next->_prev = A;
            }
        }
        // search executed set
        else if (_exec && _exec->_et > t) {
            Node* curr = _exec;
            while (curr->_prev && curr->_prev->_et >= t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t)) {
                curr = curr->_prev;
            }
            // if curr exists (not null) and correct time, remove event from list
            if (curr->_prev && curr->_prev->_et == t) {
                Node* tmp = curr->_prev;

                curr->_prev = tmp->_prev;
                
                if (tmp->_prev) {
                    tmp->_prev->_next = curr;
                }
                
                // number of rollbacks
                rollbacks++;

                // rolling back
                while (_exec != tmp->_prev)
                {
                    _exec->_ea->SendAntiMsg();
                    _exec = _exec->_prev;

                    // counting number of rolled events
                    numEventRolls++;
                }
                
                if (tmp->_prev)
                    _exec = tmp->_prev->_prev;
                else
                    _exec = 0;

                _curr = tmp->_prev;

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
                Node* A = new Node(t, ea);
                // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                A->_prev = curr->_prev;
                A->_next = curr;
                curr->_prev = A;

                if (A->_prev)
                    A->_prev->_next = A;
            }
        }    
        // if time equals either
        else if ((_curr && _curr->_et == t) || (_exec && _exec->_et == t)) {
            if ((_curr && _curr->_et == t) && (_exec && _exec->_et == t)) {
                // checking head
                if (_curr && _curr->_ea->GetEventId() == ea->GetEventId()) {
                    Node* tmp = _curr;

                    _curr = _curr->_next;
                    if (_curr)
                        _curr->_prev = _exec;

                    if (_exec)
                        _exec->_next = _curr;

                    // deleting event
                    delete tmp->_ea;
                    delete ea;

                    // deleting node
                    delete tmp;
                    tmp = 0;
                }
                else {
                    Node* curr = _curr;
                    while (curr->_next && curr->_next->_et == t && curr->_next->_ea->GetEventId() != ea->GetEventId()) {
                        curr = curr->_next;
                    }
                    // if curr exists (not null) and correct time, remove event from list
                    if (curr->_next && curr->_next->_et == t) {
                        Node* tmp = curr->_next;

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
                        if (_exec->_ea->GetEventId() == ea->GetEventId() && _exec->_et == t) {
                            Node* tmp = _exec;

                            // increment number of rollbacks
                            rollbacks++;
                            numEventRolls++;

                            // rollback
                            if (_exec->_prev) {
                                _exec->_prev->_next = _curr;
                                _exec = _exec->_prev->_prev;
                            }
                            else {
                                _exec = 0;
                            }

                            if (_curr)
                                _curr->_prev = tmp->_prev;
                            _curr = tmp->_prev;

                            // Send anti-msgs
                            tmp->_ea->SendAntiMsg();

                            // deleting event
                            delete tmp->_ea;
                            delete tmp;
                            tmp = 0;
                            delete ea;
                        }
                        else {
                            curr = _exec;
                            while (curr->_prev && curr->_prev->_et == t && curr->_prev->_ea->GetEventId() != ea->GetEventId()) {
                                curr = curr->_prev;
                            }
                            // if curr exists (not null) and correct time, remove event from list
                            if (curr->_prev && curr->_prev->_et == t) {
                                Node* tmp = curr->_prev;

                                curr->_prev = tmp->_prev;

                                if (tmp->_prev) {
                                    tmp->_prev->_next = curr;
                                }
                                
                                // incrment number of rollbacks
                                rollbacks++;

                                // rolling back
                                while (_exec != tmp->_prev)
                                {
                                    _exec->_ea->SendAntiMsg();
                                    _exec = _exec->_prev;
                                    
                                    // counting number of events rolled
                                    numEventRolls++;
                                }

                                if (tmp->_prev)
                                    _exec = tmp->_prev->_prev;
                                else
                                    _exec = 0;

                                _curr = tmp->_prev;

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
                                Node* A = new Node(t, ea);
                                // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                                A->_prev = curr->_prev;
                                A->_next = curr;
                                curr->_prev = A;

                                if (A->_prev)
                                    A->_prev->_next = A;
                            }
                        }
                    }
                }
            }
            else if (_exec && _exec->_et == t) {
                if (_exec->_ea->GetEventId() == ea->GetEventId() && _exec->_et == t) {
                    Node* tmp = _exec;

                    // rollback
                    if (_exec->_prev) {
                        _exec->_prev->_next = _curr;
                        _exec = _exec->_prev->_prev;
                    }
                    else {
                        _exec = 0;
                    }

                    if (_curr)
                        _curr->_prev = tmp->_prev;
                    _curr = tmp->_prev;

                    // Send anti-msgs
                    tmp->_ea->SendAntiMsg();
                    
                    // counting number of rollbacks
                    rollbacks++;

                    // counting number of events rolled
                    numEventRolls++;

                    // deleting event
                    delete tmp->_ea;
                    delete tmp;
                    tmp = 0;
                    delete ea;
                }
                else {
                    Node* curr = _exec;
                    while (curr->_prev && curr->_prev->_et == t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t)) {
                        curr = curr->_prev;
                    }
                    // if curr exists (not null) and correct time, remove event from list
                    if (curr->_prev && curr->_prev->_et == t) {
                        Node* tmp = curr->_prev;

                        curr->_prev = tmp->_prev;

                        if (tmp->_prev) {
                            tmp->_prev->_next = curr;
                        }

                        // rolling back
                        while (_exec != tmp->_prev)
                        {
                            _exec->_ea->SendAntiMsg();
                            _exec = _exec->_prev;

                            // counting number of events rolled
                            numEventRolls++;
                        }

                        if (tmp->_prev)
                            _exec = tmp->_prev->_prev;
                        else
                            _exec = 0;

                        _curr = tmp->_prev;

                        // Send anti-msgs
                        tmp->_ea->SendAntiMsg();

                        // counting number of events rolled
                        numEventRolls++;

                        // counting number of rollbacks
                        rollbacks++;

                        // deleting event
                        delete tmp->_ea;
                        delete tmp;
                        tmp = 0;
                        delete ea;
                    }
                    // schedule anti-msg
                    else {
                        Node* A = new Node(t, ea);
                        // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                        A->_prev = curr->_prev;
                        A->_next = curr;
                        curr->_prev = A;

                        if (A->_prev)
                            A->_prev->_next = A;
                    }
                }
            }
            // _curr time equals new event time
            else {
                // checking head
                if (_curr && _curr->_ea->GetEventId() == ea->GetEventId()) {
                    Node* tmp = _curr;

                    _curr = _curr->_next;
                    if (_curr)
                        _curr->_prev = _exec;

                    if (_exec)
                        _exec->_next = _curr;

                    // deleting event
                    delete tmp->_ea;
                    delete ea;

                    // deleting node
                    delete tmp;
                    tmp = 0;
                }
                else {
                    Node* curr = _curr;
                    while (curr->_next && curr->_next->_et == t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                        curr = curr->_next;
                    }
                    // if curr exists (not null) and correct time, remove event from list
                    if (curr->_next && curr->_next->_et == t) {
                        Node* tmp = curr->_next;

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
                        Node* A = new Node(t, ea);
                        // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                        A->_next = curr->_next;
                        A->_prev = curr;
                        curr->_next = A;

                        if (A->_next)
                            A->_next->_prev = A;
                    }
                }
            }
        }
        // if exec is null or exec->t >
        else {
            Node* A = new Node(t, ea);
            A->_next = _curr;
            A->_prev = _exec;

            if (_curr)
                _curr->_prev = A;
            if (_exec)
                _exec->_next = A;

            _exec = A;
        }
    }
    // not a anti-msg - real event
    else { 
        // if time is less than last executed time
        if (_exec && _exec->_et > t) {
            // Locate the node before the point of insertion 
            Node* curr = _exec;
            // while previous node is exist and less than t and not equal too anti-msg on that node
            while (curr->_prev &&
                curr->_prev->_et >= t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t))
            {
                curr = curr->_prev;
            }
            // if not anti-msg, schedule event
            if (!(curr->_prev && curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t)) {
                Node* new_event = new Node(t, ea);
                new_event->_next = curr;
                new_event->_prev = curr->_prev;
                curr->_prev = new_event;

                if (new_event->_prev) {
                    new_event->_prev->_next = new_event;
                }

                // rollback
                while (_exec != new_event) {
                    // sending anti-msgs
                    _exec->_ea->SendAntiMsg();
                    _exec = _exec->_prev;

                    // counting number of events rolled
                    numEventRolls++;
                }

                // counting number of rollbacks
                rollbacks++;

                // setting executed 
                _exec = new_event->_prev;

                // setting curr to new node
                _curr = new_event;

            }
            // anti-msg found, remove anti-msg and delete ea
            else {
                // remove anti-msg and do not schedule new msg
                Node* anti = curr->_prev;
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
        else if (_curr && _curr->_et < t) {
            // Locate the node before the point of insertion 
            Node* curr = _curr;
            while (curr->_next &&
                curr->_next->_et < t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t))
            {
                curr = curr->_next;
            }
            if (!(curr->_next && curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                Node* new_event = new Node(t, ea);
                new_event->_next = curr->_next;
                new_event->_prev = curr;
                
                if (curr->_next)
                    curr->_next->_prev = new_event;

                curr->_next = new_event;
            }
            // anti-msg found
            else {
                // remove anti-msg and do not schedule new msg
                Node* anti = curr->_next;
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
        else if ((_curr && _curr->_et == t) || (_exec && _exec->_et == t)) {
            if ((_curr && _curr->_et == t) && (_exec && _exec->_et == t)) {
                // searching for anti-msgs in executed set
                Node* curr = _exec;
                // while a previous event exists w/same timestamp, check for anti-msgs
                while (curr->_prev &&
                    curr->_prev->_et == t && curr->_prev->_ea->GetEventId() != ea->GetEventId())
                {
                    curr = curr->_prev;
                }
                // if anti-msg was found
                if (curr->_prev && curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t) {
                    // remove anti-msg and do not schedule new msg
                    Node* anti = curr->_prev;
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
                    curr = _curr;

                    // while next event w/same timestamp, check for anti-msgs
                    while (curr->_next &&
                        curr->_next->_et == t && curr->_next->_ea->GetEventId() != ea->GetEventId())
                    {
                        curr = curr->_next;
                    }
                    // no anti-msg found, schedule 
                    if (!(curr->_next && curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                        Node* new_event = new Node(t, ea);
                        new_event->_next = curr->_next;
                        new_event->_prev = curr;

                        if (curr->_next)
                            curr->_next->_prev = new_event;

                        curr->_next = new_event;
                    }
                    // anti-msg found
                    else {
                        // remove anti-msg and do not schedule new msg
                        Node* anti = curr->_next;
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
            }
            else if (_exec && _exec->_et == t) {
                // searching for anti-msgs in executed set
                Node* curr = _exec;
                // while a previous event exists w/same timestamp, check for anti-msgs
                while (curr->_prev &&
                    curr->_prev->_et == t && curr->_prev->_ea->GetEventId() != ea->GetEventId())
                {
                    curr = curr->_prev;
                }
                // if anti-msg was found
                if (curr->_prev && curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t) {
                    // remove anti-msg and do not schedule new msg
                    Node* anti = curr->_prev;
                    curr->_prev = anti->_prev;
                    if (anti->_prev)
                        anti->_prev->_next = curr;

                    // delete event
                    delete ea;
                    delete anti->_ea;
                    delete anti;
                    anti = 0;
                }
                // else, rollback 
                else {
                    Node* new_event = new Node(t, ea);
                    new_event->_next = curr;
                    new_event->_prev = curr->_prev;
                    curr->_prev = new_event;

                    if (new_event->_prev) {
                        new_event->_prev->_next = new_event;
                    }

                    // rollback
                    while (_exec != new_event) {
                        // sending anti-msgs
                        _exec->_ea->SendAntiMsg();
                        _exec = _exec->_prev;

                        // counting number of events rolled
                        numEventRolls++;
                    }

                    // counting number of rollbacks
                    rollbacks++;

                    // setting executed 
                    _exec = new_event->_prev;

                    // setting curr to new node
                    _curr = new_event;
                }
            }
            // _curr time equals new event time
            else {
                // Locate the node before the point of insertion 
                Node* curr = _curr;
                while (curr->_next &&
                    curr->_next->_et == t && curr->_next->_ea->GetEventId() != ea->GetEventId())
                {
                    curr = curr->_next;
                }
                if (!(curr->_next && curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                    Node* new_event = new Node(t, ea);
                    new_event->_next = curr->_next;
                    new_event->_prev = curr;

                    if (curr->_next)
                        curr->_next->_prev = new_event;

                    curr->_next = new_event;
                }
                // anti-msg found
                else {
                    // remove anti-msg and do not schedule new msg
                    Node* anti = curr->_next;
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
        }
        // if curr is either null or curr->t > t while t > exec->t
        else {
            Node* new_event = new Node(t, ea);
            new_event->_next = _curr;
            new_event->_prev = _exec;

            if (_curr)
                _curr->_prev = new_event;
            if (_exec)
                _exec->_next = new_event;

            _curr = new_event;
        }
    }
}

EventAction * EventSet::GetEventAction(){
    // move exec to next event
    _exec = _curr;

    // move curr to next event 
    _curr = _curr->_next;

    // execute next event
    return _exec->_ea;
}

Time EventSet::GetEventTime(){
    return _curr->_et;
}