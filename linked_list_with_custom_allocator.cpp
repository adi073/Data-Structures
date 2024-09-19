#include <memory>
#include <iostream>

template <typename T>
class SimpleAllocator {
public:
    using value_type = T;

    SimpleAllocator() = default;

    template <typename U>
    constexpr SimpleAllocator(const SimpleAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        std::cout << "Allocating " << n * sizeof(T) << " bytes" << std::endl;
        if (n > std::allocator_traits<SimpleAllocator>::max_size(*this)) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t) noexcept {
        std::cout << "Deallocating memory" << std::endl;
        ::operator delete(p);
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        std::cout << "Constructing element" << std::endl;
        new(p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p)   
 noexcept {
        std::cout << "Destroying element" << std::endl;
        p->~U();
    }

    friend bool operator==(const SimpleAllocator&, const SimpleAllocator&) { return true; }
    friend bool operator!=(const SimpleAllocator&, const SimpleAllocator&) { return false; }
};

template <typename T, typename Allocator = SimpleAllocator<T>>
class LinkedList {
public:
    using value_type = T;
    using allocator_type = Allocator;

    LinkedList() : head(nullptr), tail(nullptr), size(0) {}

    ~LinkedList() {
        clear();
    }

    void push_back(const T& value) {
        Node* newNode = allocator.allocate(1);
        allocator.construct(newNode, value);

        if (head == nullptr) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }

        ++size;
    }

    void clear() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;   

            allocator.destroy(current);
            allocator.deallocate(current, 1);
            current = next;
        }
        head = tail = nullptr;
        size = 0;
    }

    size_t size() const {
        return size;
    }

private:
    struct Node {
        T value;
        Node* next;
    };

    Node* head;
    Node* tail;
    size_t size;
    allocator_type allocator;
};

int main() {
    SimpleAllocator<int> alloc;

    LinkedList<int, SimpleAllocator<int>> list(alloc);

    std::cout << "Adding elements to linked list:" << std::endl;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    std::cout << "Linked list contents: ";
    for (size_t i = 0; i < list.size(); ++i) {
        std::cout << list.get(i) << " ";
    }
    std::cout << std::endl;

    std::cout << "Clearing linked list:" << std::endl;
    list.clear();

    return 0;
}
