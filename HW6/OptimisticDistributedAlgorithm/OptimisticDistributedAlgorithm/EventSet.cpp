#include "EventSet.h"

EventSet::Node::Node(){
    m_ea = nullptr;
    m_et = 0.0f;    
    m_next = nullptr;
}

EventSet::Node::Node(const Time& t, EventAction * ea){
    m_ea = ea;
    m_et = t;
    m_next = nullptr;
}

void EventSet::AddEvent(const Time& t, EventAction * ea){
    
    // testing if ea is anti-msg (event class id == 0)
    if (ea->GetEventClassId() == 0) {
        // Locate the node before the point of insertion 
        Node* current = m_head;
        if (current != nullptr && current->m_et == t && current->m_ea->GetEventId() == ea->GetEventId()) {
            // remove event from list
            EventAction* ea = current->m_ea;
            current = current->m_next;
            delete ea;
        }
        else if (current != nullptr) {
            while (current->m_next != nullptr && current->m_next->m_et <= t && current->m_next->m_ea->GetEventId() != ea->GetEventId()) {
                current = current->m_next;
            }

            // if current != null, then found Simultaneous event
            if (current->m_next != nullptr && current->m_next->m_et == t && current->m_next->m_ea->GetEventId() == ea->GetEventId()) {
                // remove event from list
                EventAction* ea = current->m_next->m_ea;
                current->m_next = current->m_next->m_next;
                delete ea;
            }
            // if event not found, wait for event
            else{
                AntiNode* currAnti = _antiHead;
                // place in list to wait for msg to arrive
                AntiNode* new_node = new AntiNode(t, ea);
                new_node->m_next = _antiHead->m_next;
                _antiHead->m_next = new_node;
            }
        }
    }
    else { 
        // determine if event is associated with anti msg waiting
        if (_antiHead) {
            AntiNode* currAnti = _antiHead;
            while (currAnti->m_next != nullptr && !(currAnti->m_next->m_et == t && currAnti->m_next->_eventId == ea->GetEventId())) {
                currAnti = currAnti->m_next;
            }
            // found
            if (currAnti->m_next != nullptr && currAnti->m_next->m_et == t && currAnti->m_next->_eventId == ea->GetEventId()) {
                AntiNode* temp = currAnti->m_next;

                currAnti->m_next = currAnti->m_next->m_next;
                delete temp;
            }
        }
        else { // no wanti-msgs waiting, place in active event set
            Node* new_node = new Node(t, ea);

            // Special case for the head end
            if (m_head == nullptr || (m_head == nullptr ? true : m_head->m_et >= new_node->m_et)) {
                new_node->m_next = m_head;
                m_head = new_node;
            }
            else {
                // Locate the node before the point of insertion 
                Node* current = m_head;
                while (current->m_next != nullptr &&
                    current->m_next->m_et < new_node->m_et)
                {
                    current = current->m_next;
                }
                new_node->m_next = current->m_next;
                current->m_next = new_node;
            }
        }
    }
}

void EventSet::isAntiMsgSimultaneous(const Time& t, unsigned int eventId)
{
    // Locate the node before the point of insertion 
    Node* current = m_head;
    if (current != nullptr && current->m_et == t && current->m_ea->GetEventId() == eventId) {
        // remove event from list
        EventAction* ea = current->m_ea;
        current = current->m_next;
        delete ea;
    }
    else if (current != nullptr) {
        while (current->m_next != nullptr && !(current->m_next->m_et == t && current->m_next->m_ea->GetEventId() == eventId)) {
            current = current->m_next;
        }

        // if current != null, then found Simultaneous event
        if (current->m_next != nullptr && current->m_next->m_et == t && current->m_next->m_ea->GetEventId() == eventId) {
            // remove event from list
            EventAction* ea = current->m_next->m_ea;
            current->m_next = current->m_next->m_next;
            delete ea;
        }
    }
}

EventAction * EventSet::GetEventAction(){
    // current head node
    Node* current = m_head;

    // swap nodes
    m_head = m_head->m_next;

    // get current event action
    EventAction* ea = current->m_ea;

    // deleting node from event set
    delete current;
    current = nullptr;

    return ea;
}

Time EventSet::GetEventTime(){
    return m_head->m_et;
}