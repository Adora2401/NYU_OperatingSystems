//
//  main.cpp
//  OS Lab2
//
//  Created by Yinuo Chang on 10/21/21.
//

#include <iostream>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <queue>
#include <stdio.h>
#include<list>
#include <algorithm>
std::vector <int> randvals;
std::pair<int, int> pairT;
int maxprio=4;
enum State {CREATED, READY, RUNNING, BLOCKED, PREEMPTION, DONE};
enum transState {TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT, TRANS_TO_DONE}; //1&4-> to_ready
int eventID = 0;
int getEventID(){
    return eventID++; //about the change of seq
}
char* stype;
class Event;


int myrandom(int burst) {
    static int ofs = 1;
        int res;
        // wrap around or step to the next
        if (ofs>randvals[0]){
          ofs = 1;
          res = (randvals[ofs] % burst) + 1;
        }
        else{
          res =(randvals[ofs++] % burst) + 1;
        }
        return res;
      }

class Process{
public:
    int AT; //Arrival Time
    int TC; //Total CPU Time
    int CB; //CPU Burst
    int IO; //IO Burst
    int cpu_burst; // Running:  the cpu_burst is defined as a random number between [ 1 .. CB ]. If the remaining execution time is smaller than the cpu_burst compute, reduce it to the remaining execution time. -> updateCpu_burst()
    int reExTime; // remaining execution time
    int io_burst; // Blocked: point the io_burst is defined as a random number between [ 1 ..IO].
    int static_priority;
    int dynamic_priority; //defined between [ 0 .. (static_priority-1) ], With every quantum expiration the dynamic priority decreases by one. When “-1” is reached the prio is reset to (static_priority-1)
    int processStartTime;  //timestamp-> timestamp+io+waiting+cpu???
    State curState = CREATED;
    bool hasBeenPree = false;
    int cpuwaitingTime = 0;
    int sum_io_time = 0;
    int timeInPrevState = 0;
    int finishtime;
    bool dynamic_priority_reset = 0;

    
    Process(int at,int tc,int cb,int io){
        AT = at;
        TC = tc;
        CB = cb;
        IO = io;
        reExTime = tc; //初始状态下，应该就是原始Total CPU Time
        processStartTime = at;
        static_priority = myrandom(maxprio);
        dynamic_priority = static_priority - 1;
        
    }
    void updateCpu_burst(){
        cpu_burst = myrandom(CB); // Ready->Running
//        cpu_burst = 50;
        if(reExTime<cpu_burst){
            cpu_burst = reExTime;
        }
    }
    void updateIO_burst(){
//        io_burst = 5;
        io_burst = myrandom(IO); //running->Blocked
    }
    
    void updatereExTime(){
        
    }
    void updateDP(){ // 用于更新dynamic_priority  every quantum expiration
        dynamic_priority = dynamic_priority - 1; // every quantum expires
        if(dynamic_priority == -1){
            dynamic_priority = static_priority - 1;
            dynamic_priority_reset = 1;
        }
        else{
            dynamic_priority_reset = 0;
        }
        
        
        
    }
};

class Scheduler{
public:
    int quantum = 10000; //for non-preemptive schedulers
    
    //还要有一个放了各种process的queue run_queue;
    virtual void add_process(Process *proc){};
    virtual Process* get_next_process(){return nullptr;};
    virtual bool test_preempt(){return 1;};//false but for E
    
};

//FCFS
// A new process is added to the end of the queue
// A blocked process that becomes ready added to the end of the queue
class FCFS: public Scheduler //First Come, First Served
{
    std::queue<Process*> runqueue;
public:
    void add_process(Process* proc){
        runqueue.push(proc);
    }
    Process* get_next_process(){
        Process* nextProcess = runqueue.front();
        runqueue.pop();
        return nextProcess;
    }
    bool test_preempt(){
        return runqueue.empty();
    }
};
//LCFS: Last Come, First Served
class LCFS: public Scheduler //First Come, Last Served
{
    std::list<Process*> runqueue;
public:
    void add_process(Process* proc){
        runqueue.push_back(proc);
    }
    Process* get_next_process(){
        Process* nextProcess = runqueue.back();
        runqueue.pop_back();
        return nextProcess;
    }
    bool test_preempt(){
        return runqueue.empty();
    }
};
//SRTF: Shortest Remaining Time Next
class SRTF_compare
  {
  public:
      bool operator() (Process* p1, Process* p2) {
        if (p1->reExTime == p2->reExTime) return p1 > p2;
        return p1->reExTime > p2->reExTime;
      }
  };
  class SRTF: public Scheduler
  { // non-preemptive version
    std::priority_queue<Process*, std::vector<Process*>, SRTF_compare> runQueue;
  public:
    void add_process(Process *proc) {
      runQueue.push(proc);
    }
    Process* get_next_process() {
      Process * temp = runQueue.top();
      runQueue.pop();
      return temp;
    }
    bool test_preempt() {return runQueue.empty();}
  };

//RR 有quantum
class RR: public Scheduler
{
  std::queue <Process*> runQueue;
public:
  RR(int q) {
      quantum = q;
  }

  void add_process(Process *proc) {
      runQueue.push(proc);
  }

  Process* get_next_process() {
      Process * temp = runQueue.front();
      runQueue.pop();
      return temp;
  }
    bool test_preempt() {return runQueue.empty();}
};

class PRIO: public Scheduler
  {
    int AQCount=0, EQCount=0;
      //Why cannot Work???
//      std::queue<Process*> activeQueue = new std::queue<Process*>[maxprio];
//    std::queue<Process*> expiredQueue = std::queue<Process*> (maxprio);
      std::vector <std::queue<Process*>> activeQueue = std::vector <std::queue<Process*>> (maxprio);
      std::vector <std::queue<Process*>> expiredQueue = std::vector<std::queue<Process*>>(maxprio);
      
    public:
      PRIO(int q) {
          quantum = q;
      }
      void add_process(Process *proc) {
          if (proc->curState == CREATED) {
            activeQueue[proc->dynamic_priority].push(proc);
            AQCount+=1;
          }else if(proc->curState == BLOCKED){
              activeQueue[proc->dynamic_priority].push(proc);
              AQCount+=1;
          } else if (proc->curState == RUNNING) {
              // quantum expires
            if (proc->dynamic_priority_reset) {//dynamic prio<0
                expiredQueue[proc->dynamic_priority].push(proc);
                EQCount+=1;

            } else {
                activeQueue[proc->dynamic_priority].push(proc);
                AQCount+=1;
            }
            proc->dynamic_priority_reset = 0;
          }
      }

      void swapAE() { //exchange
        if (AQCount!=0) return;
        std::queue<Process*> temp;
        for (int i = 0; i < maxprio; i++) {
          temp = activeQueue[i];
          activeQueue[i] = expiredQueue[i];
          expiredQueue[i] = temp;
        }
        int tempI;
        tempI=AQCount;
        AQCount=EQCount;
        EQCount=tempI;
      }

      Process* get_next_process() {
          //use test_preempt() to test aQ?=empty, swap
          int curMaxPrio = maxprio-1;
          while (curMaxPrio > -1) {
              if (activeQueue[curMaxPrio].empty()){ curMaxPrio-=1; }
            else{
              Process* temp = activeQueue[curMaxPrio].front();
              activeQueue[curMaxPrio].pop();
              AQCount-=1;
              return temp;
            }
          }
          return nullptr;
      }
      bool test_preempt(){
        if (AQCount==0) swapAE();
        return AQCount==0;
      }
  };
  
class Event{
public:
    int evtTimeStamp;
    int id;
    Process* evtProcess;
//    State oldstate = CREATED;
//    State newstate =  READY;
    transState transition;

    Event(int ts, Process* proc, transState transS,int id):id(id){
        evtTimeStamp = ts;
        evtProcess = proc;
        transition = transS;
//        id++;
    }
    
};
class EventCompare
 {
   public:
     bool operator() (Event* e1, Event* e2) {
         if(e1->evtTimeStamp == e2->evtTimeStamp) return e1->id> e2->id;
       return e1->evtTimeStamp > e2->evtTimeStamp;
     }
 };

class DES_Layer{
public:
    std::priority_queue<Event*, std::vector<Event*>, EventCompare> eventQueue;
    
    Event* get_event(){
        if(eventQueue.empty()){
            return nullptr;
        }
        else{
            return eventQueue.top();
        }
    }
    
    void put_event(Event *event){
        eventQueue.push(event);
    }
    
    void rm_event(){
        eventQueue.pop();
    }//remove_event
};



void Simulation(DES_Layer *DL,std::vector<std::pair<int, int>> &IOUse){
    Event* evt;
    Scheduler *newS;
    //假设最开始跑的是FCFS
    int quantum, maxprio;
          if (strcmp(stype, "F") == 0) {
              newS = new FCFS();
            std::cout << "FCFS" << "\n";
          } else if (strcmp(stype, "L") == 0) {
              newS = new LCFS();
              std::cout << "LCFS" << "\n";
          } else if (strcmp(stype, "S") == 0) {
              newS = new SRTF();
              std::cout << "SRTF" << "\n";
          } else if (*stype == 'R') {
              quantum = atoi(stype + 1);
              newS = new RR(quantum);
              std::cout << "RR" << " " << newS->quantum << "\n";
          }else if (*stype == 'P') {
                  sscanf(stype + 1, "%d:%d", &quantum, &maxprio);
                  newS = new PRIO(quantum);
                  std::cout << "PRIO" << " " << newS->quantum << "\n";
              } else if (*stype == 'E') {
                  sscanf(stype + 1, "%d:%d", &quantum, &maxprio);
                  newS = new PRIO(quantum);
                  std::cout << "PREPRIO" << " " << newS->quantum << "\n";
              }

    
    Process *CURRENT_RUNNING_PROCESS = nullptr; //瞎定义的
    bool CALL_SCHEDULER = false;
    int CURRENT_TIME;
//    int j = 0;
    while((evt = DL->get_event())){
//        printf("这是第 %d 个模拟 \n",++j);
//        printf("EVENT ID %d\n",eventID);
//        std::cout<<"现在的TimeStamp: "<<evt->evtTimeStamp<<'\n';
        DL->rm_event();
        Process *proc = evt->evtProcess; //This is process the event works on
        CURRENT_TIME = evt->evtTimeStamp; //Time jumps discretely;
        proc->timeInPrevState = CURRENT_TIME - proc->processStartTime;
        proc->processStartTime = CURRENT_TIME;
//        std::cout<<"Event Trans: "<<"proc curState; "<<'\n';
//        std::cout<<evt->transition<<" "<<proc->curState<<'\n';
        Event* newE;
        

//            case TRANS_TO_PREEMPT:
////                std::cout<<"TRANS_TO_PREEMPT"<< '\n';
//                //add to run queue (no event is generated)
//                //not available to those non-pree
//                proc->reExTime = proc->reExTime - proc->timeInPrevState;
//                proc->cpu_burst -= newS->quantum;
//                CURRENT_RUNNING_PROCESS = nullptr;
//
//                proc->updateDP();
//                //对时间戳存疑
//                newE = new Event(CURRENT_TIME+proc->timeInPrevState,proc,TRANS_TO_READY);
//                DL->put_event(newE);
//                CALL_SCHEDULER = true;
//                break;
//        }

//
        switch(evt->transition) {
                    case TRANS_TO_READY:
                        // must come from BLOCKED or from PREEMPTION
                        if (proc->curState == BLOCKED) {
                          proc->sum_io_time += proc->timeInPrevState;
                          proc->io_burst -= proc->timeInPrevState;
                          proc->dynamic_priority = proc->static_priority-1;
                        }
                        // must add to run_queue
                        newS->add_process(proc);
                        proc->curState = READY;
                        CALL_SCHEDULER = true;
                        break;

                    case TRANS_TO_RUN:
                      if (proc->hasBeenPree == 0) {
                        proc->updateCpu_burst();
                      }
                      else {proc->hasBeenPree = 0;}
                      CURRENT_RUNNING_PROCESS = proc;
                      if (proc->curState==READY) proc->cpuwaitingTime += proc->timeInPrevState;
                      proc->curState = RUNNING;
                      if (proc->cpu_burst <= newS->quantum){
                        if (proc->reExTime <= proc->cpu_burst){
                          newE = new Event(CURRENT_TIME + proc->reExTime, proc, TRANS_TO_DONE,getEventID());
                        }
                        else{
                          newE = new Event(CURRENT_TIME + proc->cpu_burst, proc, TRANS_TO_BLOCK,getEventID());
                        }
                      }
                      else{
                          if(newS->quantum>=proc->reExTime){
                              newE = new Event(CURRENT_TIME + proc->reExTime, proc, TRANS_TO_DONE,getEventID());
                          }
                          else{
                          newE = new Event(CURRENT_TIME + newS->quantum, proc, TRANS_TO_PREEMPT,getEventID());
                              proc->hasBeenPree = 1;}
                      }

                      DL->put_event(newE);
                      break;

                    case TRANS_TO_BLOCK:
                      if (proc->curState!=RUNNING) {
                        //cout<<"ERROR IN TRANS!";
                      }
                        CURRENT_RUNNING_PROCESS = nullptr;
                        proc->updateIO_burst();
                        proc->cpu_burst -= proc->timeInPrevState;
                        proc->reExTime -= proc->timeInPrevState;
                        proc->curState = BLOCKED;

                        pairT.first=CURRENT_TIME;
                        pairT.second=CURRENT_TIME + proc->io_burst;
                        IOUse.push_back(pairT);
                        
                        newE = new Event(CURRENT_TIME + proc->io_burst, proc, TRANS_TO_READY,getEventID());
                        DL->put_event(newE);
                        CALL_SCHEDULER = true;
                        break;
                    case TRANS_TO_DONE:
                      if (CURRENT_RUNNING_PROCESS!=proc) continue;
                      proc->reExTime = 0;
                      proc->curState = DONE;
                      CURRENT_RUNNING_PROCESS = nullptr;
                      CALL_SCHEDULER = true;
                      break;
                    case TRANS_TO_PREEMPT:
                      proc->reExTime -= proc->timeInPrevState;
                      proc->cpu_burst -= newS->quantum;
                      CURRENT_RUNNING_PROCESS = nullptr;
                      proc->updateDP();
                      newS->add_process(proc);
                      proc->curState = READY;
                      CALL_SCHEDULER = true;
                      break;


                  }

                  delete evt;
                  evt = nullptr;
                  
        if (CALL_SCHEDULER) {
            Event* nextEvent= DL->get_event();
            if (CURRENT_RUNNING_PROCESS != nullptr) {
                if(*stype=='E' &&CURRENT_RUNNING_PROCESS->dynamic_priority < proc->dynamic_priority&&nextEvent->evtTimeStamp!=CURRENT_TIME){
                    newE = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS,TRANS_TO_PREEMPT,getEventID());
                    DL->put_event(newE);
                    CURRENT_RUNNING_PROCESS->hasBeenPree = 1;
                    continue;
                }
                else{continue;}
              
              }
            if (nextEvent!=nullptr && nextEvent->evtTimeStamp == CURRENT_TIME) continue;
            CALL_SCHEDULER = false;
            if (CURRENT_RUNNING_PROCESS == nullptr && !newS->test_preempt()) {
                CURRENT_RUNNING_PROCESS = newS->get_next_process();
                  
                // run the process
                newE = new Event(CURRENT_TIME,CURRENT_RUNNING_PROCESS,TRANS_TO_RUN,getEventID());
                DL->put_event(newE);
                  
              }
          }

    }
    delete newS;
}

void output(std::vector<Process> allProc,std::vector<std::pair<int, int>> &IOUse){
    int ftAllProc=0;
    double time_cpubusy, Avg_turnaround, Avg_CPUWait;
    int time_iobusy = 0;
    for(int i=0;i<allProc.size();i++){
        int FinishTime = allProc[i].AT+allProc[i].sum_io_time+allProc[i].cpuwaitingTime+allProc[i].TC;
        allProc[i].finishtime = FinishTime;
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", i, allProc[i].AT,allProc[i].TC,allProc[i].CB,allProc[i].IO,allProc[i].static_priority,FinishTime,FinishTime-allProc[i].AT,allProc[i].sum_io_time,allProc[i].cpuwaitingTime);
        ftAllProc = std::max(ftAllProc,FinishTime);
        time_cpubusy += allProc[i].TC;
        Avg_turnaround += FinishTime-allProc[i].AT;
        Avg_CPUWait += allProc[i].cpuwaitingTime;
    }
    sort(IOUse.begin(), IOUse.end());
    
    if (!IOUse.empty()) {
        int start = IOUse[0].first, end = IOUse[0].second;
        for(auto x = IOUse.begin(); x!=IOUse.end(); ++x){
          if (end<x->first){
            time_iobusy += end - start;
            start = x->first;
            end = x->second;
          }
          else end = std::max(end, x->second);
        }
        time_iobusy += end - start;
    }
    
    double cpu_util = 100.0* time_cpubusy/(double)ftAllProc;
    double io_util = 100.0 * time_iobusy/(double)ftAllProc;
    double num_processes = allProc.size();
    Avg_turnaround /=num_processes;
    Avg_CPUWait /= num_processes;
    double throughput = 100.0 * num_processes/(double)ftAllProc;
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
           ftAllProc, cpu_util, io_util, Avg_turnaround, Avg_CPUWait, throughput);
    
}

int main(int argc, char * argv[]) {
    int o,a;
    int vflag = 0;
    const char *optstring = "vtes:";
    while ((o = getopt(argc, argv, optstring)) != -1) {
        switch(o)
        {
          case 'v':
              vflag = 1;
              break;
          case 't': break;
          case 'e': break;
          case 's':
              stype = optarg;
              sscanf(stype+1, "%d:%d", &a, &maxprio);
              break;
        }
    }
//    std::ifstream randfile("/Users/yinuochang/Downloads/lab2_assign/rfile");
    std::ifstream randfile(argv[optind+1]);
    int temp;
    randfile >> temp;
    randvals.push_back(temp);
    for (int a = 1; a < randvals[0]; a++) {
      randfile >> temp;
      randvals.push_back(temp);
    }
    std::vector<Process> allProc;
//    std::string str ="/Users/yinuochang/Downloads/lab2_assign/input0";
//    std::ifstream fin(str);
    std::ifstream fin(argv[optind]);
    std::string s;
    while(getline(fin,s)){
        std::string pro[4];
        std::istringstream input(s);
        input>>pro[0]>>pro[1]>>pro[2]>>pro[3];
        Process newProcess = Process(stoi(pro[0]),stoi(pro[1]),stoi(pro[2]),stoi(pro[3]));
        allProc.push_back(newProcess);
    }
    
    Event* newEVT;
    DES_Layer* newDES = new DES_Layer();
    
    int i;
    for(i=0;i<allProc.size();i++){
        newEVT = new Event(allProc[i].AT,&allProc[i],TRANS_TO_READY,getEventID());
        newDES->put_event(newEVT);
    }
    
    std::vector<std::pair<int, int>> IOUse;
    Simulation(newDES,IOUse);
    output(allProc,IOUse);


}
