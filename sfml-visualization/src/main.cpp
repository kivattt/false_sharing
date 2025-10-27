// We assume 64-byte cache line size
// Check the file /sys/devices/system/cpu/cpu0/cache/coherency_line_size

#include <iostream>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <immintrin.h>
#include <SFML/Graphics.hpp>

using namespace std::chrono; // i'm so sick of it
using std::vector;

typedef unsigned char byte;
typedef unsigned long long ULL;

std::mutex m;
int volatile counter = 0;

void do_stuff(byte* theByte, int core) {
	*theByte = 0;

//	for (int i = 0; i < 500000070; i++) {
	for (int i = 0; i < 50070; i++) {
		asm volatile("" ::: "memory");
		*theByte += 1;
	}

	byte expected = 150; // 134
	if (*theByte != expected) {
		std::cerr << "Panic! Value wasn't " << int(expected) << "...\n";
		std::cerr << "It was: " << int(*theByte) << '\n';
		exit(0);
	}

	m.lock();
	counter--;
	m.unlock();
}

vector<long> get_timings_per_offset(byte *data) {
	vector<long> timings_nanoseconds;
	for (int distanceBetweenBytes = 1; distanceBetweenBytes < 128; distanceBetweenBytes++) {
		m.lock();
		counter = 2;
		m.unlock();

		std::thread t1(do_stuff, data, 1);
		std::thread t2(do_stuff, data + distanceBetweenBytes, 2);
		steady_clock::time_point start = steady_clock::now();

		while (counter != 0) {}

		steady_clock::time_point end = steady_clock::now();
		long duration_nanos = duration_cast<nanoseconds>(end - start).count();
		timings_nanoseconds.push_back(duration_nanos);

		t1.join();
		t2.join();
	}

	return timings_nanoseconds;
}

void draw_timings(sf::RenderWindow &window, vector<long> &timings) {
	sf::CircleShape circle(1.0f);
	//circle.setFillColor(sf::Color(255, 255, 255, 1));
	circle.setFillColor(sf::Color(255, 255, 255, 80));

	float step = window.getSize().x / timings.size();
	float x = 0.0;
	for (int i = 0; i < timings.size(); i++) {
		int y = window.getSize().y - (timings[i] / 200);
		//int y = 500 + window.getSize().y - (timings[i] / 50); // For 3840x2160 resolution
		circle.setPosition({x, y});
		window.draw(circle);

		x += step;
	}
}

int main() {
	sf::RenderWindow window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "specs");
	window.setVerticalSyncEnabled(true);

	// Aligned to cache-line size
	byte* data = (byte*)std::aligned_alloc(64, 128);
	if ((unsigned long long)data % 64 != 0) {
		std::cerr << "Allocation was somehow not aligned to 64 bytes...\n";
		return 1;
	}

	std::cout << &data << '\n';

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
			window.clear();
		}

		vector<long> timings_nanoseconds = get_timings_per_offset(data);
        //window.clear();
		draw_timings(window, timings_nanoseconds);
        window.display();
    }

	std::free(data);
}
