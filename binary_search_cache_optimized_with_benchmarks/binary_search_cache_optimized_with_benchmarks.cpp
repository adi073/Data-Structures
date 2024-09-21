#include <benchmark/benchmark.h> 
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdint>
#include "utils.hpp"
int simple_binary_search(int* array, int number_of_elements, int key) {
    int low = 0, high = number_of_elements - 1, mid;
    while (low <= high) {
        mid = (low + high) / 2;

        if (array[mid] < key)
            low = mid + 1;
        else if (array[mid] > key)
            high = mid - 1;
        else {
            return mid;
        }
    }
    return -1;
}

template <int cache_line_size>
int* get_block_start(int* array, int block_mask) {
    uintptr_t arr_ptr = (uintptr_t)array;
    arr_ptr = arr_ptr & block_mask;

    if (arr_ptr != (uintptr_t)array) {
        arr_ptr += cache_line_size;
    }
    return (int*)arr_ptr;
}

template <int cache_line_size>
int* get_block_end(int* array, int len, int block_mask) {
    uintptr_t arr_ptr = (uintptr_t)(array + len - 1);
    uintptr_t arr_ptr_len = (uintptr_t)(array + len);

    int* end_block_start =
        get_block_start<cache_line_size>(array + len, block_mask);
    int* end_block_start2 =
        get_block_start<cache_line_size>(array + len - 1, block_mask);

    if (end_block_start == end_block_start2) {
        return end_block_start - (cache_line_size / sizeof(int));
    } else {
        return end_block_start2;
    }
}
template <int cache_line_size = 64>
int cachefriendly_binary_search(int* array, int number_of_elements, int key) {
    int low_block, high_block, mid_block;
    static constexpr int block_mask = ~(cache_line_size - 1);
    static constexpr int ints_in_block = (cache_line_size / sizeof(int));

    struct block {
        unsigned char x[cache_line_size];
    };

    const block* block_base_addr =
        (block*)get_block_start<cache_line_size>(array, block_mask);
    low_block = 0;
    int last_block = high_block = ((block*)get_block_end<cache_line_size>(
                                      array, number_of_elements, block_mask)) -
                                  block_base_addr;

    while (low_block <= high_block) {
        mid_block = (low_block + high_block) / 2;

        int* mid_low = (int*)(block_base_addr + mid_block);
        int* mid_high = ((int*)(block_base_addr + mid_block + 1)) - 1;

        if (key < *mid_low) {
            high_block = mid_block - 1;
        } else if (key > *mid_high) {
            low_block = mid_block + 1;
        } else {
            int* p = mid_low;
            while (p <= mid_high) {
                if (*p == key) {
                    return (p - array);
                }
                p++;
            }
            return -1;
        }
    }

    if (low_block == 0) {
        int* p = array;
        while (p != (int*)block_base_addr) {
            if (*p == key) {
                return (p - array);
            }
            p++;
        }
        return -1;
    } else if (high_block == last_block) {
        int* end = array + number_of_elements;
        int* p = (int*)(block_base_addr + last_block);
        while (p < end) {
            if (*p == key) {
                return (p - array);
            }
            p++;
        }
        return -1;
    } else {
        return -1;
    }
}
//static constexpr int ARR_LEN = 1024 * 1024 * 100;
static constexpr int ARR_LEN = 1024 * 100;
//static constexpr int ARR_LEN = 16;

//int main(int argc, char** argv) {
////    std::vector<int> test_array = create_random_array<int>(ARR_LEN, 10, ARR_LEN / 2);
////    std::sort(test_array.begin(), test_array.end());
//
//    std::vector<int> test_array = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
//    
//  int index = simple_binary_search(&test_array[0], test_array.size(), 5);
//    std::cout << "Index = " << index << ", val = " << test_array[index] << std::endl;
//
//    index = cachefriendly_binary_search(&test_array[0], test_array.size(), 5);
//    std::cout << "Cache friendly index = " << index << ", val = " << test_array[index] << std::endl;
//
//}

static void BM_SimpleBinarySearch(benchmark::State& state) {
    std::vector<int> test_array = create_random_array<int>(ARR_LEN, 10, ARR_LEN / 2);
    //std::vector<int> test_array = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    std::sort(test_array.begin(), test_array.end());

    int key = test_array[ARR_LEN / 2];

    for (auto _ : state) {
        int index = simple_binary_search(&test_array[0], test_array.size(), key);
    }
}

static void BM_CacheFriendlyBinarySearch(benchmark::State& state) {
    std::vector<int> test_array = create_random_array<int>(ARR_LEN, 10, ARR_LEN / 2);
    std::sort(test_array.begin(), test_array.end());
//    std::vector<int> test_array = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

    int key = test_array[ARR_LEN / 2];

    for (auto _ : state) {
        int index = cachefriendly_binary_search(&test_array[0], test_array.size(), key);
    }
}

BENCHMARK(BM_SimpleBinarySearch)->RangeMultiplier(2)->Range(1<<1, 1<<6);
BENCHMARK(BM_CacheFriendlyBinarySearch)->RangeMultiplier(2)->Range(1<<1, 1<<6);

BENCHMARK_MAIN();

