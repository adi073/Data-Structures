#include <atomic>
#include <cstddef>
#include <iostream>
#include <thread>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <string>
#include <vector>
#include <iterator>
#include <string>
#include <chrono>
#include <algorithm>
#include <memory>

// Linux stuff
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

template<typename Element, size_t Size> 
class CircularFifo{
public:
    enum { Capacity = Size+1 };

    CircularFifo() : _tail(0), _head(0){}   
    virtual ~CircularFifo() {}

    bool push(const Element& item);
    bool pop(Element& item);

    bool wasEmpty() const;
    bool wasFull() const;
    bool isLockFree() const;

private:
    size_t increment(size_t idx) const; 
    std::atomic <size_t>  _tail;
    Element    _array[Capacity];
    std::atomic<size_t>   _head;
};

template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::push(const Element& item)
{	
    const auto current_tail = _tail.load(std::memory_order_relaxed); 
    const auto next_tail = increment(current_tail); 
    if(next_tail != _head.load(std::memory_order_acquire))                           
    {	
        _array[current_tail] = item;
        _tail.store(next_tail, std::memory_order_release); 
        return true;
    }
    return false; // full queue
}

// Pop by Consumer can only update the head (load with relaxed, store with release)
//     the tail must be accessed with at least aquire
template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::pop(Element& item)
{
    const auto current_head = _head.load(std::memory_order_relaxed);  
    if(current_head == _tail.load(std::memory_order_acquire)) 
        return false; // empty queue

    item = _array[current_head]; 
    _head.store(increment(current_head), std::memory_order_release); 
    return true;
}

template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::wasEmpty() const
{
    // snapshot with acceptance of that this comparison operation is not atomic
    return (_head.load() == _tail.load()); 
}


// snapshot with acceptance that this comparison is not atomic
template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::wasFull() const
{
    const auto next_tail = increment(_tail.load()); // aquire, we dont know who call
    return (next_tail == _head.load());
}


template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::isLockFree() const
{
    return (_tail.is_lock_free() && _head.is_lock_free());
}

template<typename Element, size_t Size>
size_t CircularFifo<Element, Size>::increment(size_t idx) const
{
    return (idx + 1) % Capacity;
}

// Producer function (can be run in a separate thread for concurrency)
void producer(CircularFifo<int, 200> &fifo) {
    for (int i = 0; i < 10; ++i) { // Produce 15 integers
        if (fifo.push(i)) {
            std::cout << "Producer pushed: " << i  << std::endl;
        } else {
            std::cerr << "Producer failed to push: FIFO full" << std::endl;
        }
    }
}

// Consumer function (can be run in a separate thread for concurrency)
void consumer(CircularFifo<int, 200> &fifo, unsigned long long int sum) {
    int data;
    while (fifo.pop(data)) {
        std::cout << "Consumer popped: " << data << std::endl;
    }
    std::cout << "Consumer reached end of queue." << std::endl;
	std::cout << "Total sum is :" << sum << std::endl;
}


int main() {
	unsigned long long int sum = 0;

    CircularFifo<int, 200> fifo;

//    producer(fifo);
//    consumer(fifo, sum);
    std::thread producerThread(producer, std::ref(fifo));
    std::thread consumerThread(consumer, std::ref(fifo), 0);
    producerThread.join();
    consumerThread.join();
    return 0;
}
