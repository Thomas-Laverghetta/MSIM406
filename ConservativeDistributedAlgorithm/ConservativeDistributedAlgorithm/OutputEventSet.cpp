#include "OutputEventSet.h"
#include "OutputEventSet.h"

OutputEventSet::Node::Node() {
    m_ea = nullptr;
    m_et = 0.0f;
    m_next = nullptr;
}

OutputEventSet::Node::Node(int LP, const Time& t, EventAction* ea) {
    m_ea = ea;
    m_et = t;
    _LP = LP;
    m_next = nullptr;
}

void OutputEventSet::AddEvent(const Time& t, EventAction* ea, int LP) {
    Node* new_node = new Node(LP, t, ea);

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
    m_nodeCounter++;
}

EventAction* OutputEventSet::GetEventAction() {
    // current head node
    Node* current = m_head;

    // swap nodes
    m_head = m_head->m_next;

    // get current event action
    EventAction* ea = current->m_ea;

    // deleting node from event set
    delete current;
    current = nullptr;
    m_nodeCounter--;

    return ea;
}

Time OutputEventSet::GetEventTime() {
    return m_head->m_et;
}

int OutputEventSet::GetLP()
{
    return m_head->_LP;
}
