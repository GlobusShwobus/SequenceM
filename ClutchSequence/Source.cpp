#include "SequenceM.h"
#include "Stopwatch.h"
#include <iostream>
#include <algorithm>

int main() {
	using namespace badEngine;
	struct memeTester {
		int lol = 69;
	};
	SequenceM<int> meme = { 3,5,7,9,2,1 };

	std::sort(meme.begin(), meme.end(), std::greater<>());


	return 0;
}