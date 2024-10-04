#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>
#include <vector>

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

class ProdCon {
public:
    void producer(int i) {
        lock_guard<mutex> lockGuard(mtx);
        Q.push(i);
        to_pool.notify_one();
    }

    void consumer() {
        while(true) {
            unique_lock<mutex> lockGuard(mtx);
            to_pool.wait(lockGuard, [this] { return !run || !Q.empty(); });

            if(!run) break;

            cout << this_thread::get_id() << ' ' << "Consumed " << Q.front() << endl;
            Q.pop();

            to_producer.notify_one();
        }
    };

    void stop() {
        lock_guard<mutex> lockGuard(mtx);
        run = false;
        to_pool.notify_all();
    }

    void wait_for_all_work_to_be_done() {
        std::unique_lock<mutex> lg(mtx);
        to_producer.wait(lg, [this] { return Q.empty(); });
    }

private:
    bool run = true;
    mutex mtx;
    condition_variable to_pool;
    condition_variable to_producer;
    queue<int> Q;
};

int main() {
    unsigned MAX_THREADS = std::thread::hardware_concurrency() - 1;
    vector<thread> ThreadVector;
    ThreadVector.reserve(MAX_THREADS);

    ProdCon prodconObj;
    for(unsigned i = 0; i < MAX_THREADS; i++) {
        ThreadVector.emplace_back(&ProdCon::consumer, &prodconObj);

//        cout << "Pool threadID:" << ThreadVector[i].get_id() << endl;
    }

    for(int i = 0; i < MAX_THREADS / 2; i++) {
        prodconObj.producer(i);
    }

    prodconObj.wait_for_all_work_to_be_done();

    // stop pool threads
    prodconObj.stop();

    for(auto&& t : ThreadVector) t.join();
	
}
