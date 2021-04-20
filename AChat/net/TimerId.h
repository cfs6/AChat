
#ifndef TIMERID_H_
#define TIMERID_H_

namespace net
{
	class Timer;
	class TimerId
	{
	public:
		TimerId(Timer* timer_, int64_t seq):timer(timer_), sequence(seq){}

		TimerId():timer(nullptr), sequence(0){}

		Timer* getTimer()
		{
			return timer;
		}
		// default copy-ctor, dtor and assignment are okay

		friend class TimerQueue;
	private:
		Timer* timer;
		int64_t sequence;
	};
}



#endif /* TIMERID_H_ */
