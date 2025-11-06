#include "SequenceM.h"
#include "Stopwatch.h"
#include <iostream>

int main() {
	using namespace badEngine;
	struct memeTester {
		int lol = 69;
	};
	SequenceM<memeTester> meme;
	meme.emplace_back(5);

	SequenceM<memeTester>::iterator ass = meme.begin();

	return 0;
}