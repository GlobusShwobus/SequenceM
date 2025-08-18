#include "Stopwatch.h"
std::chrono::duration<float> Stopwatch::Mark() {
	const auto old = last;
	last = std::chrono::steady_clock::now();
	const std::chrono::duration<float> frametime = last - old;
	return frametime;
}
Stopwatch::Stopwatch() {
	last = std::chrono::steady_clock::now();
}

float Stopwatch::MarkFloat() {
	auto time = Mark();
	return time.count();
}
std::chrono::milliseconds Stopwatch::MarkMilliSec() {
	auto time = Mark();
	return std::chrono::duration_cast<std::chrono::milliseconds>(time);
}
std::chrono::microseconds Stopwatch::MarkMicroSec() {
	auto time = Mark();
	return std::chrono::duration_cast<std::chrono::microseconds>(time);
}

uint32_t FrameTimer::getFPS()const
{
	return fps;
}
void FrameTimer::setFPS(uint32_t FPS)
{
	fps = FPS;
	limit = std::chrono::duration<float>(ONE_SECOND / fps);
}
float FrameTimer::getLimitFloat()const {
	return limit.count();
}
std::chrono::milliseconds FrameTimer::getLimitMilliSec()const {
	return std::chrono::duration_cast<std::chrono::milliseconds>(limit);
}
std::chrono::microseconds FrameTimer::getLimitMicroSec()const {
	return std::chrono::duration_cast<std::chrono::microseconds>(limit);
}