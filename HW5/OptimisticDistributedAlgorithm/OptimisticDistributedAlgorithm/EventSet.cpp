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
            m_nodeCounter--;
        }
        else if (current != nullptr) {
            while (current->m_next != nullptr && current->m_next->m_et <= t && !(current->m_next->m_et == t && current->m_next->m_ea->GetEventId() == ea->GetEventId())) {
                current = current->m_next;
            }

            // if current != null, then found Simultaneous event
            if (current->m_next != nullptr && current->m_next->m_et == t && current->m_next->m_ea->GetEventId() == ea->GetEventId()) {
                // remove event from list
                EventAction* ea = current->m_next->m_ea;
                current->m_next = current->m_next->m_next;
                delete ea;
                m_nodeCounter--;
            }
            // if current->m_next->m_et > t
            else if (current->m_next != nullptr) {
                // place in list to wait for msg to arrive
                Node* new_node = new Node(t, ea);
                new_node->m_next = current->m_next;
                current->m_next = new_node;
            }
        }
    }
    else {
        Node* new_node = new Node(t, ea);

        // Special case for the head end
        if (m_head == nullptr || (m_head == nullptr ? true : m_head->m_et >= new_node->m_et)) {
            new_node->m_next = m_head;
            m_currPos = m_head = new_node;
        }
        else {
            // Locate the node before the point of insertion 
            Node* current = m_head;
            while (current->m_next != nullptr &&
                current->m_next->m_et <= new_node->m_et 
                && (current->m_next->m_et == new_node->m_et ? current->m_next->m_ea->GetEventId() != new_node->m_ea->GetEventId() : true))
            {
                current = current->m_next;
            }

            // if waiting anti-msg was found
            if (current->m_next->m_ea->GetEventId() == new_node->m_ea->GetEventId()) {
                EventAction* ea = current->m_next->m_ea;
                current->m_next = current->m_next->m_next;
                delete ea;
                m_nodeCounter--;
                return;
            }
            
            new_node->m_next = current->m_next;
            current->m_next = new_node;
        }
        m_nodeCounter++;
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
        m_nodeCounter--;
    }
    else if (current != nullptr) {
        while (current->m_next != nullptr && !(current->m_next->m_et == t && current->m_next->m_ea->GetEventId() == eventId)) {
            current = current->m_next;
        }

        // if current != null, then found Simultaneous event
        if (current->m_next != nullptr) {
            // remove event from list
            EventAction* ea = current->m_next->m_ea;
            current->m_next = current->m_next->m_next;
            delete ea;
            m_nodeCounter--;
        }
    }
}

EventAction * EventSet::GetEventAction(){
    // current head node
    Node * current = m_currPos;
    
    // determining if next node is anti-msg
    if (m_currPos->m_next != nullptr && m_currPos->m_next->m_ea->GetEventId() != 0) {
        if (m_currPos == m_head) {
            m_currPos = m_head = m_currPos->m_next;
        }
        else {
            m_currPos = m_currPos->m_next;
        }
    }
    // anti-msg next
    else if (m_currPos->m_next != nullptr) {
        // iterate until next is not anti-msg (find pos msg)
        Node* curr = nullptr;
        if (m_currPos == m_head)
            curr = m_head = m_currPos->m_next;
        else
            curr = m_currPos->m_next;

        while (curr != nullptr && curr->m_ea->GetEventId() == 0) {
            curr = curr->m_next;
        }
        m_currPos = curr;
    }
    else {
        m_currPos = m_head = nullptr;
    }
    
    // get current event action
    EventAction * ea = current->m_ea;

    // deleting node from event set
    delete current;
    current = nullptr;
    m_nodeCounter--;

    return ea; 
}

Time EventSet::GetEventTime(){
    return m_head->m_et;
}