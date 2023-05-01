#pragma once
#include <chrono>

using HiresClockT = std::chrono::high_resolution_clock;
using TimePointT = HiresClockT::time_point;
using DurationT = HiresClockT::duration;

struct Stopwatch
{
	void start()
	{
		_start = HiresClockT::now();
	}

	void stop()
	{
		_stop = std::chrono::high_resolution_clock::now();

		_elapsed = _stop - _start;
	}

	DurationT elapsed() const
	{
		return _elapsed;
	}

private:
	TimePointT _start;
	TimePointT _stop;
	DurationT _elapsed {};
};