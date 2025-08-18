#pragma once
#include <chrono>
class Stopwatch {
	std::chrono::steady_clock::time_point last;

	std::chrono::duration<float> Mark();

public:
	Stopwatch();

	float MarkFloat();
	std::chrono::milliseconds MarkMilliSec();
	std::chrono::microseconds MarkMicroSec();
};

class FrameTimer : public Stopwatch {
	static constexpr float ONE_SECOND = 1.0f;
	static constexpr uint32_t defaultFPS = 60;

	std::chrono::duration<float> limit;
	uint32_t fps = 0;

public:
	FrameTimer(uint32_t fps = defaultFPS) :fps(fps), limit(ONE_SECOND / fps) {}

	uint32_t getFPS()const;
	void setFPS(uint32_t FPS);
	float getLimitFloat()const;
	std::chrono::milliseconds getLimitMilliSec()const;
	std::chrono::microseconds getLimitMicroSec()const;
};