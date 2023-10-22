#pragma once



class Clock
{
public:
	Clock(): curr_time(0) {
		curr_time = 0;
	};
	void add_time(int units) {
		curr_time += units;
	};
	int time(){
		return curr_time;
	};
	void reset() {
		curr_time = 0;
	}
	void set(int t) {
		curr_time = t;
	}
private:
	int curr_time;
};

#ifndef haveaclock
#define haveaclock
extern Clock timer;
#endif
