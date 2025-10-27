// We assume 64-byte cache line size
// Check the file /sys/devices/system/cpu/cpu0/cache/coherency_line_size

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <immintrin.h>

using namespace std::chrono; // i'm so sick of it
using std::vector;

typedef unsigned char byte;
typedef unsigned long long ULL;

// This function was written by Github copilot
void restrict_this_thread_to_cpu_core(int core) {
	return;

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core, &cpuset);
	pthread_t current_thread = pthread_self();
	int rc = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	if (rc != 0) {
		std::cerr << "Error: " << rc << '\n';
		return;
	}
}

std::mutex m;
int volatile counter = 0;

void do_stuff(byte* theByte, int core) {
	restrict_this_thread_to_cpu_core(core);

	*theByte = 0;

//	for (int i = 0; i < 500000070; i++) {
	for (int i = 0; i < 500070; i++) {
		asm volatile("" ::: "memory");
		*theByte += 1;
	}

	byte expected = 102; // 134
	if (*theByte != expected) {
		std::cerr << "Panic! Value wasn't " << int(expected) << "...\n";
		std::cerr << "It was: " << int(*theByte) << '\n';
		exit(0);
	}

	m.lock();
	counter--;
	m.unlock();
}

vector<long> get_timings_per_offset() {
	// Aligned to cache-line size
	byte* data = (byte*)aligned_alloc(64, 128);

	vector<long> timings_nanoseconds;
	for (int distanceBetweenBytes = 1; distanceBetweenBytes < 128; distanceBetweenBytes++) {
	//for (int distanceBetweenBytes = 63; distanceBetweenBytes < 128; distanceBetweenBytes++) {
		m.lock();
		counter = 2;
		m.unlock();

		std::thread t1(do_stuff, data, 1);
		std::thread t2(do_stuff, data + distanceBetweenBytes, 2);
		steady_clock::time_point start = steady_clock::now();

		while (counter != 0) {
			//asm volatile("" ::: "memory");
		}

		steady_clock::time_point end = steady_clock::now();
		long duration_nanos = duration_cast<nanoseconds>(end - start).count();
		timings_nanoseconds.push_back(duration_nanos);

		t1.join();
		t2.join();
		
		// TODO: Do some trick so the compiler won't eliminate our dead code, instead of this:
		//std::cerr << int(*data) << '\n';
		//std::cerr << int(*(data + distanceBetweenBytes)) << '\n';
	}

	free(data);
	return timings_nanoseconds;
}

int main() {
	// Pins the main thread to core 0, for our spinlock
	restrict_this_thread_to_cpu_core(0);

	vector<long> timings_nanoseconds = get_timings_per_offset();

	std::cout << "Address offset (bytes),Nanoseconds\n";
	for (int i = 0; i < timings_nanoseconds.size(); i++) {
		std::cout << i+1 << "," << timings_nanoseconds[i] << "\n";
	}

	long largestNegativeDiff = 0;
	int indexOfLargestNegativeDiff = 0;
	long lastE = 0;
	//for (long &e : timings_nanoseconds) {
	for (int i = 0; i < timings_nanoseconds.size(); i++) {
		long e = timings_nanoseconds[i];
		long diff = e - lastE;
		if (diff < largestNegativeDiff) {
			largestNegativeDiff = diff;
			indexOfLargestNegativeDiff = i;
		}
		lastE = e;
	}

	std::cout << "Predicted cache line size: " << 1+indexOfLargestNegativeDiff << '\n';

	return 0;
}
