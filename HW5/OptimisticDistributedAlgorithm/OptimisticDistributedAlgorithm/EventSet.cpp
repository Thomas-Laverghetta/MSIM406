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
    if (ea->GetEventClassId() == 0) {
        if (_curr && _curr->_et <= t) {
            if (_curr && _curr->_ea->GetEventId() == ea->GetEventId()) {
                Node* tmp = _curr;

                _curr = _curr->_next;
                if (_curr)
                    _curr->_prev = _exec;

                if (_exec)
                    _exec->_next = _curr;

                // deleting event
                delete tmp->_ea;
                delete tmp;
                tmp = 0;
            }
            else {
                Node * curr = _curr;
                while (curr->_next && curr->_next->_et <= t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                    curr = curr->_next;
                }
                // if curr exists (not null) and correct time, remove event from list
                if (curr->_next && curr->_next->_et == t) {
                    Node* tmp = curr->_next;

                    curr->_next = curr->_next->_next;

                    if (tmp->_next) {
                        tmp->_next->_prev = curr;
                    }

                    // deleting event
                    delete tmp->_ea;
                    delete tmp;
                    tmp = 0;
                }
                // schedule anti-msg
                else {
                    Node* new_node = new Node(t, ea);
                    // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                    new_node->_next = curr;
                    new_node->_prev = curr->_prev;

                    // if there exists a previous executed event
                    if (curr->_prev) {
                        curr->_prev->_next = new_node;   // repointing previous event to next event relative to this event
                    }
                    if (curr->_next) {
                        curr->_next->_prev = new_node;   // repointing next event to previous event ralative to this event
                    }
                }
            }
        }
        // search executed set
        else {
            if (!_exec) { // either no executed events w/no sch events or just no executed events
                // schedule anti-msg
                Node* new_node = new Node(t, ea);
                // set new_node's pointers to exec
                _exec = new_node;
                _exec->_next = _curr;

                if (_curr) {
                    _curr->_prev = _exec; 
                }
            }
            else if (_exec->_ea->GetEventId() == ea->GetEventId()) {
                Node* tmp = _exec;

                // rollback
                _exec = _exec->_prev;
                if (_exec)
                    _exec->_next = _curr;

                if (_curr)
                    _curr->_prev = _exec;

                // Send anti-msgs
                tmp->_ea->SendAntiMsg();

                // deleting event
                delete tmp->_ea;
                delete tmp;
                tmp = 0;
            }
            else {
                Node* curr = _exec;
                while (curr->_prev && curr->_prev->_et <= t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t)) {
                    curr = curr->_prev;
                }
                // if curr exists (not null) and correct time, remove event from list
                if (curr->_prev && curr->_prev->_et == t) {
                    Node* tmp = curr->_prev;


                    // rollback
                    curr->_prev = tmp->_prev;
                    
                    if (tmp->_prev) {
                        tmp->_prev->_next = curr;
                        _exec = tmp->_prev;
                    }
                    else {
                        _exec = 0;
                    }
                    _curr = curr;
                   
                    // send anti-msgs 
                    tmp->_ea->SendAntiMsg();

                    // deleting event
                    delete tmp->_ea;
                    delete tmp;
                    tmp = 0;
                }
                // schedule anti-msg
                else {
                    Node* new_node = new Node(t, ea);
                    // set new_node's pointers to curr since curr is at one event lower than larger timestamp/null
                    new_node->_prev = curr;
                    new_node->_next = curr->_next;

                    // if there exists a previous executed event
                    if (curr->_next) {
                        curr->_next->_prev = new_node;   // repointing previous event to next event relative to this event
                    }
                    if (curr->_prev) {
                        curr->_prev->_next = new_node;   // repointing next event to previous event ralative to this event
                    }
                }
            }
        }
        
    }
    else { 
        // if time is less than last executed time
        if (_exec && _exec->_et > t) {
            // Locate the node before the point of insertion 
            Node* curr = _exec;
            while (curr->_prev &&
                curr->_prev->_et < t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t))
            {
                curr = curr->_prev;
            }
            if (!(curr->_prev && curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t)) {
                Node* new_node = new Node(t, ea);
                new_node->_next = curr;
                new_node->_prev = curr->_prev;

                if (curr->_prev)
                    curr->_prev->_next = new_node;

                curr->_prev = new_node;

                // rollback
                Node* tmp = _exec;
                while (tmp != new_node) {
                    // sending anti-msgs
                    tmp->_ea->SendAntiMsg();
                    tmp = tmp->_next;
                }

                // setting curr to new node
                _curr = new_node;
                _exec = _curr->_prev;

            }
            // anti-msg found
            else {
                // remove anti-msg and do not schedule new msg
                Node* anti = curr->_prev;
                curr->_prev = anti->_prev;
                if (anti->_prev)
                    anti->_prev->_next = curr;

                // delete event
                delete ea;
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
                Node* new_node = new Node(t, ea);
                new_node->_next = curr->_next;
                new_node->_prev = curr;
                
                if (curr->_next)
                    curr->_next->_prev = new_node;

                curr->_next = new_node;
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
            }
        }
        // inbetween next event and previous event
        else if (_curr && _curr->_et > t && _exec && _exec->_et < t) {
            Node* new_node = new Node(t, ea);
            new_node->_next = _curr;
            _curr->_prev = new_node;
            _curr = new_node;

            new_node->_prev = _exec;
            _exec->_next = new_node;            
        }
        // if time equals either
        else if ((_curr && _curr->_et == t) || (_exec && _exec->_et == t)) {
            if ((_curr && _curr->_et == t) && (_exec && _exec->_et == t)) {
                Node* curr = _exec;
                while (curr->_prev &&
                    curr->_prev->_et < t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t))
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
                }
                // search scheduled events
                else {
                    curr = _curr;
                    while (curr->_next &&
                        curr->_next->_et == t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t))
                    {
                        curr = curr->_next;
                    }
                    // no anti-msg found, schedule 
                    if (!(curr->_next && curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                        Node* new_node = new Node(t, ea);
                        new_node->_next = curr->_next;
                        new_node->_prev = curr;

                        if (curr->_next)
                            curr->_next->_prev = new_node;

                        curr->_next = new_node;
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
                    }
                }
            }
            else if (_exec && _exec->_et == t) {
                Node* curr = _exec;
                while (curr->_prev &&
                    curr->_prev->_et < t && !(curr->_prev->_ea->GetEventId() == ea->GetEventId() && curr->_prev->_et == t))
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
                }
                // else, rollback 
                else {
                    Node* new_node = new Node(t, ea);
                    new_node->_next = curr;
                    new_node->_prev = curr->_prev;

                    if (curr->_prev)
                        curr->_prev->_next = new_node;

                    curr->_prev = new_node;

                    // rollback
                    Node* tmp = _exec;
                    while (tmp != new_node) {
                        // sending anti-msgs
                        tmp->_ea->SendAntiMsg();
                        tmp = tmp->_next;
                    }

                    // setting curr to new node
                    _curr = new_node;
                    _exec = _curr->_prev;
                }
            }
            // _curr time equals new event time
            else {
                // Locate the node before the point of insertion 
                Node* curr = _curr;
                while (curr->_next &&
                    curr->_next->_et == t && !(curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t))
                {
                    curr = curr->_next;
                }
                if (!(curr->_next && curr->_next->_ea->GetEventId() == ea->GetEventId() && curr->_next->_et == t)) {
                    Node* new_node = new Node(t, ea);
                    new_node->_next = curr->_next;
                    new_node->_prev = curr;

                    if (curr->_next)
                        curr->_next->_prev = new_node;

                    curr->_next = new_node;
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
                }
            }
        }
        // if curr equals null
        else {
            Node* new_node = new Node(t, ea);
            _curr = new_node;
            _curr->_prev = _exec;
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