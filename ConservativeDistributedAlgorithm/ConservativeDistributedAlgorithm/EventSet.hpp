#ifndef EVENT_SET_H
#define EVENT_SET_H
#include "SimulationExecutive.h"
/*
    Thomas Laverghetta's Binary Search Tree Event Set
*/
class EventSet
{
private:
    // Event Set data
    class Node {
    public:
        Node* _LC;
        Node* _RC;
        Node* _P;
        EventAction* _ea;
        Time _et;

        Node(const Time& t, EventAction* ea) : _LC(nullptr), _RC(nullptr), _P(nullptr), _ea(ea), _et(t) {}

        ~Node() {
            delete _LC;
            delete _RC;
            delete _ea;

            _LC = nullptr;
            _RC = nullptr;
            _ea = nullptr;
        }
    };
    // root of BST
    Node* _root;

    // smallest node in the BST
    Node* _small;

    // size of the event set
    unsigned int _nodeCounter;
public:
    // saves event
    void AddEvent(const Time& t, EventAction* ea) {
        Node* leaf = new Node(t, ea);
        _nodeCounter++;

        // no nodes in system
        if (!_root) {
            _root = _small = leaf;
            return;
        }

        // iterator
        Node* curr = _root;

        // will break via return on conditional branches below
        while (true) {
            // if leaf is greater or equal to iterator, and if true iterate through RC
            if (leaf->_et >= curr->_et) {
                if (curr->_RC) // if RC exists
                    curr = curr->_RC;
                else { // if !RC exists, then set iterator RC too leaf and return
                    curr->_RC = leaf;
                    leaf->_P = curr;
                    return;
                }
            }
            else { // leaf is less than curr
                if (curr->_LC) // 
                    curr = curr->_LC;
                else {
                    curr->_LC = leaf;
                    leaf->_P = curr;
                    if (curr == _small)
                        _small = leaf;
                    return;
                }
            }
        }
    }

    // Returns size of Event Set
    unsigned int EventSetSize() {
        return _nodeCounter;
    }

    bool HasEvents() {
        return _nodeCounter != 0;
    }

    // returns event with smallest time 
    EventAction* GetEventAction() {
        EventAction* ea = _small->_ea;
        Node* deleteNode = _small;
        if (_small->_RC) {
            if (_small == _root) {
                _root = _small->_RC;
                _small->_RC->_P = nullptr;
            }
            else {
                _small->_P->_LC = _small->_RC;
                _small->_RC->_P = _small->_P;
            }

            // iterating until LC is the smallest
            _small = _small->_RC;

            while (_small->_LC)
                _small = _small->_LC;
        }
        else if (_small == _root)
            _small = _root = nullptr;
        else {
            _small = _small->_P;
            _small->_LC = nullptr;
        }
        // deleting node
        deleteNode->_RC = nullptr; // setting null due to Node::~Node()
        deleteNode->_ea = nullptr;
        delete deleteNode;
        deleteNode = nullptr;

        _nodeCounter--;
        return ea;
    }

    // returns time with smallest time stamp and deletes event from set
    Time GetEventTime() {
        return _small->_et;
    }

    // Defualt constructor
    inline EventSet() {
        _root = nullptr;
        _small = nullptr;
        _nodeCounter = 0;
    }

    // deletes all nodes 
    ~EventSet() {
        delete _root;
        _root = nullptr;
        _small = nullptr;
        _nodeCounter = 0;
    }
};
#endif // !EVENT_SET_H

