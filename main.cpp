#include <stdio.h>
#include "clock.h"
#include <string>
#include "Process.h"
#include <concurrent_priority_queue.h>
#include <queue>
#include <iostream>
#include <fstream>
using namespace std;


const bool TESTING{ false }; //for debugging mode

Proc_queue ready;
Proc_queue ready2(rr10);
Proc_queue ready3(fcfs);
IO_queue io_waiting;
Proc_queue done;
Proc_queue runningq;
Clock timer;

void load_data(vector<vector<int>>& test_data) {
	if (TESTING) {
		test_data.push_back({ 100 });
		test_data.push_back({ 2, 2, 2, 2, 2 });
	}
	else {
		test_data.push_back({ 5, 27, 3, 31, 5, 43, 4, 18, 6, 22, 4, 26, 3, 24, 4 });
		test_data.push_back({ 4, 48, 5, 44, 7, 42, 12, 37, 9, 76, 4, 41, 9, 31, 7, 43, 8 });
		test_data.push_back({ 8, 33, 12, 41, 18, 65, 14, 21, 4, 61, 15, 18, 14, 26, 5, 31, 6 });
		test_data.push_back({ 3, 35, 4, 41, 5, 45, 3, 51, 4, 61, 5, 54, 6, 82, 5, 77, 3 });
		test_data.push_back({ 16, 24, 17, 21, 5, 36, 16, 26, 7, 31, 13, 28, 11, 21, 6, 13, 3, 11, 4 });
		test_data.push_back({ 11, 22, 4, 8, 5, 10, 6, 12, 7, 14, 9, 18, 12, 24, 15, 30, 8 });
		test_data.push_back({ 14, 46, 17, 41, 11, 42, 15, 21, 4, 32, 7, 19, 16, 33, 10 });
		test_data.push_back({ 4, 14, 5, 33, 6, 51, 14, 73, 16, 87, 6 });
	}
	return;
}

void report() {
	
	//outputs report on all current lists
	cout << "Time: " << timer.time() <<endl;
	//output current process
	if (runningq.is_empty()) {
		cout << "CPU idle. \n";
	} else {
		cout << "Current process running: " << runningq.first->pid << " with " << runningq.first->currburst << " time units left." << endl;
	}
	//output io queue
	cout << "IO Queue: " << endl;  //nb: update "last time & currburst, print pid & currburst
	io_waiting.report();
	cout << "Ready Queue: \n";
	ready.report();
	if (ready2.first != (Process*)nullptr) {
		cout << "Ready queue 2: \n";
		ready2.report();
	}
	if (ready3.first != (Process*)nullptr) {
		cout << "Ready queue 3: \n";
		ready3.report();
	}
}

void update_stats() {
	Process* temp {nullptr};
	//updates statistics on each process given new time
	//update running process
	temp = runningq.first;
	if (temp != (Process*)nullptr) {
		temp->update();
		temp = temp->next_proc;
	}	
	//update waiting processes
	temp = ready.first;
	while (temp != (Process*)nullptr) {
		temp->update();
		temp = temp->next_proc;
	}
	temp = ready2.first;
	while (temp != (Process*)nullptr) {
		temp->update();
		temp = temp->next_proc;
	}
	temp = ready3.first;
	while (temp != (Process*)nullptr) {
		temp->update();
		temp = temp->next_proc;
	}
	//update io processes
	temp = io_waiting.first;
	while (temp != (Process*)nullptr) {
		temp->update();
		temp = temp->next_proc;
	}
}

void round_robin() {
	Process* temp{ nullptr };
	runningq.enqueue(ready.dequeue());
	report();
	int rundone{ INT_MAX };
	int iodone{ INT_MAX };
	
	while (!(ready.is_empty()) || !(io_waiting.is_empty()) || !(runningq.is_empty())) { //what if queues empty???
		if (runningq.is_empty() && !(ready.is_empty())) { //sending process to cpu
			runningq.enqueue(ready.dequeue());
			cout << "Time: " << timer.time() << "; Sending process " << runningq.first->pid << " to cpu.\n\n";
			report();
			continue; 
		}
		rundone = INT_MAX;
		iodone = INT_MAX;
		if (!io_waiting.is_empty()) { iodone = io_waiting.first->t_interrupt; }
		if (!runningq.is_empty()) { rundone = runningq.first->t_interrupt; }
		if (iodone <= rundone) { //io finished
			timer.set(io_waiting.first->t_interrupt);
			update_stats();
			cout << "\n\nTime " << timer.time() << endl;
			cout << "Process " << io_waiting.first->pid << " completed io.  sending to ready queue \n\n";
			ready.enqueue(io_waiting.dequeue());
			report();
			continue;
		} else { //pulling process from cpu
			//cpu burst done???
			timer.set(runningq.first->t_interrupt);
			update_stats();
			if (runningq.first->burst_done()) { //cpu burst complete
				if (runningq.first->all_done()) {  //no more bursts
					cout << "\n\nTime " << timer.time() << endl;
					cout << "Process " << runningq.first->pid << " completed final cpu burst.  sending to done list \n\n";
					done.enqueue(runningq.dequeue());
					report();
					continue;
				}	else { //
					cout << "\n\nTime " << timer.time() << endl;
					cout << "Process " << runningq.first->pid << " completed cpu burst.  sending to io \n\n";
					io_waiting.enqueue(runningq.dequeue());
					report();
					continue;

				}
			}
			else { //cpu burst interrupted
				cout << "\n\nTime " << timer.time() << endl;
				cout << "Process " << runningq.first->pid << " finished allocated cpu time (did not finish burst).  sending to ready queue \n\n";
				ready.enqueue(runningq.dequeue());
				report();
				continue;
			}\
		
		}
		
	}
};


void mlfq_io_enq() {
	//sends finished io process to correct ready queue
	cout << "\n\nTime " << timer.time() << endl;
	if (io_waiting.first->tq == 5) { ready.enqueue(io_waiting.dequeue()); }
	else if (io_waiting.first->tq == 10) { ready2.enqueue(io_waiting.dequeue()); }
	else { ready3.enqueue(io_waiting.dequeue()); }
	cout << "Sent to ready queue.\n\n";
};

void mlfq_cpu_enq(bool interrupted=false) {
	//sends interrupted or finished cpu process to correct ready queue
	if (runningq.first->all_done()) { //current process is finished
		cout << "\n\nTime " << timer.time() << endl;
		cout << "Process " << runningq.first->pid << " completed final cpu burst.  sending to done list \n\n";
		done.enqueue(runningq.dequeue());
		return;
	}
	else {  //current process goes to io or ready queue
		
		
		if (runningq.first->burst_done()) { //burst done-send to io
			cout << "sending process " << runningq.first->pid << " to io.\n";
			io_waiting.enqueue(runningq.dequeue());
		}
		else { //not done--//send to appropriate queue
			
			cout << "Process " << runningq.first->pid << " did not complete cpu burst.  sending to queue \n\n";
			if (!interrupted) { //time ran out--demote
				cout << " demoting process\n";
				runningq.first->downgrade();
			}
			switch (runningq.first->tq) {
			case 5:
				ready.enqueue(runningq.dequeue());
				break;
			case 10:
				ready2.enqueue(runningq.dequeue());
				break;
			case 100:
				ready3.enqueue(runningq.dequeue());
				break;

			}
		}
	}
}



void mlfq_cpu_choose(){
	//chooses proces for cpu
	if (!(ready.is_empty())) { runningq.enqueue(ready.dequeue()); } //choose correct queue
	else if (!(ready2.is_empty())) { runningq.enqueue(ready2.dequeue()); }
	else { runningq.enqueue(ready3.dequeue()); };
	cout << "Time: " << timer.time() << "; Sending process " << runningq.first->pid << " to cpu.  Tq: " << runningq.first->tq << "\n\n";

}


void mlfq() {
	Process* temp{ nullptr };
	runningq.enqueue(ready.dequeue());
	report();
	int rundone{ INT_MAX };
	int iodone{ INT_MAX };

	while (!(ready.is_empty()) || !(ready2.is_empty()) || !(ready3.is_empty()) || !(io_waiting.is_empty()) || !(runningq.is_empty())) { //there is at least 1 active process
		if (runningq.is_empty() && (!(ready.is_empty()) || !(ready2.is_empty()) || !(ready3.is_empty()))) { //sending process to cpu
			mlfq_cpu_choose();
		}
		rundone = INT_MAX;
		iodone = INT_MAX;
		if (!io_waiting.is_empty()) { iodone = io_waiting.first->t_interrupt; }
		if (!runningq.is_empty()) { rundone = runningq.first->t_interrupt; } //find earliest next interrupt
		//new logic
		if (iodone == rundone) {  //io & active process both finished
			timer.set(iodone);
			update_stats();
			cout << "Process " << io_waiting.first->pid << " completed io and process " << runningq.first->pid << " finished burst.\n\n";
			mlfq_cpu_enq(false);
			mlfq_io_enq();
			
		}

		if (iodone < rundone) { //io finished
			timer.set(iodone);
			update_stats();
			cout << "\n\nTime " << timer.time() << endl;
			cout << "Process " << io_waiting.first->pid << " completed io.  \n";
			//should it interrupt running process?
			if ((runningq.first != nullptr) && io_waiting.first->tq < runningq.first->tq) { //higher priority--replace currntly running process
				//current process pre-empted-send to correct queue--nb: send to front?????
				mlfq_cpu_enq(true); //if this is false, cpu utilization goes to 95.8%!
				mlfq_io_enq();
			} else mlfq_io_enq();
			
		}  
		
		if (rundone<iodone) { //cpu process done 
			timer.set(rundone);
			update_stats();
			cout << "\n\nTime " << timer.time() << endl;
			cout << "Process " << runningq.first->pid << " leaving cpu.  ";
			mlfq_cpu_enq(true);
		}
		report();
	}

}

void final_report() {
	//reports on test and deletes processes
	//***alter to output to file as well!
	cout << "\n\nTest complete.  Total runtime " << timer.time() << endl;
	float avg_wait{ 0 };
	float avg_response{ 0 };
	float avg_turnaround{ 0 };
	float running{ 0 };
	int last_done{ 0 };
	int processes_counted{ 0 };
	Process* temp{ nullptr };
	while (done.first != (Process*)nullptr) {
		processes_counted++;
		temp = done.first;
		avg_wait += (float)(done.first->waitingtime-avg_wait) / processes_counted;
		avg_response += (float)(done.first->responsetime-avg_response) / processes_counted;
		avg_turnaround += (float)(done.first->finishtime-avg_turnaround) / processes_counted;
		running += (done.first->runtime);
		last_done = max(last_done, done.first->finishtime);
		cout << "Process " << temp->pid  << ":  \n     Turnaround time : " << done.first->finishtime <<"\n     Running time : " << done.first->runtime << "\n     IO time : " << done.first->iotime << "\n     Waiting time : " << done.first->waitingtime << "\n     Response time : " << done.first->responsetime << endl<<endl;
		done.first = done.first->next_proc;
		delete temp;
	}
	cout << "Averages:" << endl;
	cout << "Waiting time: " << avg_wait << endl;
	cout << "Turnaround time: " << avg_turnaround << endl;
	cout << "Response time: " << avg_response << endl;
	cout << "CPU Utilization: " << (running / last_done) << endl << endl; 
};



int main() {
	
	//redirect output to file
	ofstream myfile;
	cout << "Output file name? ";
	string filename;
	cin  >> filename;
	std::ofstream ofs{filename};
	auto cout_buff = std::cout.rdbuf();
	std::cout.rdbuf(ofs.rdbuf());
	

	//set up queues
	io_waiting.queue_state = io;
	done.queue_state = completed;
	runningq.queue_state = running;
	vector<vector<int>> test_data;
	load_data(test_data);
	cout << "processes loaded\n";
	//load all processes into waiting
	Queue_type qtype{ sbf }; 
	Process* temp{ (Process*)nullptr };
	
	
	for (int i = 0; i < test_data.size(); i++) {
		temp = new Process(test_data[i]);
		temp->change_state(waiting);
		ready.enqueue(temp);
	}
	cout << "Ready queue loaded\n";
	
	
	cout << "running FCFS algorithm\n";
	
	round_robin();
	final_report();
	
	cout << "running sjf algorithm\n";
	//load all processes into waiting
	
	ready.reset_type(qtype);
	for (int i = 0; i < test_data.size(); i++) {
		temp = new Process(test_data[i]);
		temp->change_state(waiting);
		ready.enqueue(temp);
	}
	
	cout << "Ready queue loaded\n";
	
	timer.reset();
	round_robin();
	final_report();

	
 	cout << "running mlfq algorithm\n";
	qtype=rr5;
	ready.reset_type(qtype);
	for (int i = 0; i < test_data.size(); i++) {
		temp = new Process(test_data[i],5); //start with tq=5
		temp->change_state(waiting);
		ready.enqueue(temp);
	}
	
	cout << "Ready queue loaded\n";
	
	timer.reset();
	mlfq();
	final_report();

	
	
	std::cout.rdbuf(cout_buff);
	
	return 0;

}