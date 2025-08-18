#include "SequenceM.h"
#include "Stopwatch.h"
#include <iostream>
struct example1 {
	example1() = delete;
	example1(int x) {}
};

using namespace lmnop;

int main() {
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

	{
		Stopwatch clock;
		SequenceM<int> test;
		for (int i = 0; i < 1000; i++) {
			test.reserve(test.capacity() + 1);
		}
		auto time = clock.MarkMicroSec();
		std::cout << time.count() << '\n';
	}


	_CrtDumpMemoryLeaks();
	return 0;
}