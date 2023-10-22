#pragma once
#include <vector>
#include<cassert>
#include <iostream>
#include <queue>
#include "Clock.h"

using namespace std;
static int num_processes = 0;
enum State { newp, waiting, running, io, completed };


class Process
{
public:
	
	
	struct Report {
		int responsetime;
		int runtime;
		int iotime;
		int waitingtime;
	};
	Process(vector<int>& burst_times, int newtq=100) : pid(-1), mystate(newp), lastchange(0),tq(0), finishtime(0), t_interrupt(0), next_proc(nullptr), waitingtime(0), myqueue(1), runtime(0), iotime(0),  currburst(0), responsetime(INT16_MAX), newprocess(true) {
		pid = ++num_processes;
		//load burst times into a queue
		for (int i = 0; i < burst_times.size(); i++) { bursttimes.push(burst_times[i]); }
		tq = newtq;
	};
	~Process() {
		num_processes--;
	}
	State status() {
		return mystate;
	};
	Report report() {
		Report results;
		results.responsetime = responsetime;
		results.runtime = runtime;
		results.iotime = iotime;
		results.waitingtime = waitingtime;
		return results;
	}
	void downgrade() {
		if (tq == 5) { tq = 10; }
		else { tq = 100; };
		myqueue++;
	}
	void update() {
		//updates time stats to new clock time
		switch (mystate) {
		case newp:
			break;
		case running:
			runtime += (timer.time() - lastchange);
			currburst -= (timer.time() - lastchange);
			lastchange = timer.time();
			break;
		case waiting:
			waitingtime += (timer.time() - lastchange);
			lastchange = timer.time();
			break;
		case io:
			iotime += (timer.time() - lastchange);
			currburst -= (timer.time() - lastchange);
			lastchange = timer.time();
			break;

		}
	}


	bool all_done() {
		return bursttimes.empty();
	}
	bool burst_done() {
		return (timer.time() == lastchange + currburst);
	}

	
	void change_state(State newstate) { 
		switch (newstate) {
		case waiting:
			t_interrupt = 0;
			if (this->mystate == io) { 
				lastchange = timer.time();
				currburst = 0;
			}
			break;
		case running:
			if (newprocess) {
				newprocess = false;
				responsetime = timer.time();
			}
			if (currburst == 0) { //new cpu burst--get new cpu burst
				currburst = bursttimes.front();  //error: popping empty deque!
				bursttimes.pop();
			}
			next_proc = (Process*)nullptr;
			t_interrupt = min((tq + timer.time()), timer.time() + currburst);  //nb: round robin policy only
			break;
		case io:
			//nb: always comes straight from running
			currburst = bursttimes.front();
			bursttimes.pop();
			t_interrupt = currburst + timer.time(); 
		case completed:
			//nb: always comes from running
			finishtime = timer.time();
		}
		this->mystate = newstate;
	}
	Process* next_proc;
	int currburst;  //stores what is left of current io or cpu burst, including current activity
	int responsetime;
	int runtime;
	int waitingtime;
	int iotime;
	int myqueue;
	int finishtime;
	int pid;
	int t_interrupt; //stores upcoming interrupt time of current cpu burst.
					//resets to 0=no current cpu burst when burst finishes
	queue<int> bursttimes;
	int tq;
private:

	State mystate;

	bool newprocess;

	int lastchange; //records time of last state change
};

enum Queue_type { fcfs, rr5, rr10, sbf };

class Proc_queue {
public:

	Proc_queue(Queue_type myalg=fcfs) : first(nullptr), last(nullptr) {
		queue_state = waiting;
		mytype = myalg;
		return;
	}
	~Proc_queue() {
		//clean up memory by deleting all the processes
		Process* temp = first;
		Process* trailer = nullptr;
		while (temp != (Process*)nullptr) {
			trailer = temp;
			temp = temp->next_proc;
			delete trailer;
		}
	}
	bool is_empty() {
		return (first == (Process*)nullptr);
	}

	void report() {
		Process* temp;
		if (first == (Process*)nullptr) {
			cout << "queue empty \n";
		}
		else {
			temp = first;
			while (temp != (Process*)nullptr) {
				switch (queue_state) {
				case running:
					cout << "Process " << temp->pid << " has " << temp->currburst << " left until finished. \n";
					break;
				case waiting: 
					if (temp->currburst > 0) {
						cout << "Process " << temp->pid << " waiting to resume, with " << temp->currburst << " left until finished current burst. \n";
					}
					else {
						cout << "Process " << temp->pid << " waiting to start new cpu burst, which will take " << temp->bursttimes.front() <<" time units\n";
					}
					break;
				case io:
					if (temp->currburst > 0) {
						cout << "Process " << temp->pid << " has " << temp->currburst << " left until finished. \n";
					}
					else {
						cout << "Process " << temp->pid << " finished io\n";
					}
				}
				temp = temp->next_proc;
			}
			
		}
	}
	void enqueue(Process* process) {
		switch (mytype) {
		case fcfs:
		case rr5:
		case rr10:
			enqueuerr(process);
			return;
		case sbf:
			enqueuesbf(process);
			return;
		}
	}
	Process* dequeue() {
		Process* temp = first;
		if (temp == nullptr) return nullptr;
		first = first->next_proc;
		temp->next_proc = (Process*)nullptr;
		if (first == nullptr) { last = nullptr; };
		return temp;
	}
	void reset_type(Queue_type newtype) {
		mytype = newtype;
		/*switch (newtype) {  //TQ is saved in process, not queue
		case rr5:
			T_quant = 5;
			break;
		case rr10:
			T_quant = 10;
			break;
		case fcfs:
			T_quant = 100;
			break;
		}*/
	}
	Process* first;
	Process* last;
	State queue_state;
private:
	
	void enqueuerr(Process* process) {
		//puts process in back of given queue
		if (process == nullptr) return;
		process->change_state(queue_state);
		if (first == (Process*)nullptr) {
			process->next_proc = (Process*)nullptr;
			first = process;
			last = process;
		}
		else {
			last->next_proc = process;
			last = process;
			process->next_proc = (Process*)nullptr;
		}
	}
	void enqueuesbf(Process* process) {
		//puts process in back of given queue
		if (process == nullptr) return;
		process->change_state(queue_state);
		if (first == (Process*)nullptr) {
			process->next_proc = (Process*)nullptr;
			first = process;
			last = process;
		}
		else {  //find right place
			if (process->bursttimes.front() < first->bursttimes.front()) {  //new process in start of queue
				process->next_proc = first;
				first = process;
			}
			else {
				Process* temp = first;
				while (temp->next_proc != (Process*)nullptr && (temp->next_proc->bursttimes.front() < process->bursttimes.front())) {
					temp = temp->next_proc;
				}
				process->next_proc = temp->next_proc;
				temp->next_proc = process;
				if (temp == last) {
					last = process;
				}

			}
		}
	}
	Queue_type mytype;
};

class IO_queue : public Proc_queue {
public:

	void enqueue(Process* process) {
		//inserts process in order by t_interrupt
		process->change_state(io);
		if (first == (Process*)nullptr) {
			process->next_proc = (Process*)nullptr;
			first = process;
			last = process;
		}
		else {
			if (first->t_interrupt > process->t_interrupt) { //put at beginning
				process->next_proc = first;
				first = process;
			}
			else  //find right place
			{
				Process* temp = first;
				while (temp->next_proc != (Process*)nullptr && (temp->next_proc->t_interrupt < process->t_interrupt)) {
					temp = temp->next_proc;
				}
				process->next_proc = temp->next_proc;
				temp->next_proc = process;
			}
		}
	}
};