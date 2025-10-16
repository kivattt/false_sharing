// We assume 64-byte cache line size
// Check the file /sys/devices/system/cpu/cpu0/cache/coherency_line_size

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <immintrin.h>

using namespace std::chrono; // i'm so sick of it
using std::vector;

typedef unsigned char byte;

void do_stuff(byte* theByte) {
	*theByte = 0;

	for (int i = 0; i < 500000070; i++) {
		asm volatile("" ::: "memory");
		*theByte += 1;
	}

	if (*theByte != 70) {
		std::cerr << "Panic! Value wasn't 70...\n";
		std::cerr << "It was: " << int(*theByte) << '\n';
		exit(0);
	}
}

int main() {
	// Aligned to cache-line size
	byte* data = (byte*)aligned_alloc(64, 128);

	vector<long> timings_nanoseconds;
	for (int distanceBetweenBytes = 1; distanceBetweenBytes < 128; distanceBetweenBytes++) {
		steady_clock::time_point start = steady_clock::now();

		std::thread t1(do_stuff, data);
		std::thread t2(do_stuff, data + distanceBetweenBytes);
		t1.join();
		t2.join();

		steady_clock::time_point end = steady_clock::now();

		long duration_nanos = duration_cast<nanoseconds>(end - start).count();
		timings_nanoseconds.push_back(duration_nanos);

		// TODO: Do some trick so the compiler won't eliminate our dead code, instead of this:
		std::cerr << int(*data) << '\n';
		std::cerr << int(*(data + distanceBetweenBytes)) << '\n';
	}

	std::cout << "Address offset (bytes),Nanoseconds\n";
	for (int i = 0; i < timings_nanoseconds.size(); i++) {
		std::cout << i+1 << "," << timings_nanoseconds[i] << "\n";
	}

	free(data);
	return 0;
}
