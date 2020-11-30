#ifndef TEMPLATE_STACK_H
#define TEMPLATE_STACK_H

template <typename T>
class Stack
{
private:
	struct Node {
		Node* _next;
		Node* _previous;
		T _data;

		Node(T data) : _data(data), _next(0), _previous(0) {}
	};

	// 
	Node* _head;
	Node* _tail;
public:
	Stack() {
		_head = 0;
		_tail = 0;
	}
	~Stack() {
		while (!_head) {
			Node* tmp = _head;
			_head = _head->_next;
			delete tmp;
			tmp = 0;
		}
	}

	//
	void push(T data) {
		Node* new_node = new Node(data);
		if (!_head) {
			_tail = _head = new_node;
		}
		else {
			_head->_next = new_node;
			_head->_previous = _head;
			_head = new_node;
		}
	}

	// 
	T top() { return _head->_data;  }
	T bottom() { return _tail->data;  }

	// 
	bool empty() { return !_head; }

	// 
	void pop() {
		Node* tmp = _head;

		_head = _head->_next;

		delete tmp;
		tmp = 0;
	}
	
	// 
	void popLeft() {
		Node* tmp = _tail;

		_tail = _tail->_previous;

		delete tmp;
		tmp = 0;
	}
};

#endif // !TEMPLATE_STACK_H
