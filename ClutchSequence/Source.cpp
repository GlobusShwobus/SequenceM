#include "SequenceM.h"
#include "Stopwatch.h"
#include <iostream>
struct rect {
	int x, y, w, h;
	rect() = default;
	rect(int X, int Y, int W, int H) :x(X), y(Y), w(W), h(H){}
};

using namespace lmnop;
int main() {
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

	const int kek = 100000;
	{
		SequenceM<rect> lol;
		lol.set_capacity(kek);
		Stopwatch watch;

		for (int i = 0; i < kek; i++) {
			lol.element_create(i, i, i, i);
		}
		lol.depricate_ordered(lol.begin() + 1000, lol.begin() + 50000);
		std::cout << "size: " << lol.size_in_use() << " total: " << lol.size_total() << " reserve: " << lol.size_reserve() << "\n";
		lol.shrink_to(lol.size_in_use());
		std::cout << "size: " << lol.size_in_use() << " total: " << lol.size_total() << " reserve: " << lol.size_reserve() << "\n";
		lol.clear();
		std::cout << "size: " << lol.size_in_use() << " total: " << lol.size_total() << " reserve: " << lol.size_reserve() << "\n";
		lol.set_reserve_size(1000);
		std::cout << "size: " << lol.size_in_use() << " total: " << lol.size_total() << " reserve: " << lol.size_reserve() << "\n";
		for (int i = 0; i < 100; i++) {
			lol.element_assign(rect{ i,i,i,i });
		}
		lol.shrink_to_fit();
		std::cout << "size: " << lol.size_in_use() << " total: " << lol.size_total() << " reserve: " << lol.size_reserve() << "\n";
		lol.depricate_unordered([](const rect& r) {return r.x > 50; });
		std::cout << "size: " << lol.size_in_use() << " total: " << lol.size_total() << " reserve: " << lol.size_reserve() << "\n";
	}


	_CrtDumpMemoryLeaks();
	return 0;
}