//
//  main.cpp
//  IOScheduler
//
//  Created by Yinuo Chang on 12/01/21.
//

#include <iostream>
#include <queue>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <cmath>
#include <string>
#include <climits>
#include <algorithm>
#include <stdlib.h>
using namespace std;

char *optarg;
int head = 0;
int direction = 1; //up 1 down -1
//use to count request No.
class ioReq{
public:
    int ioNo;
    int trackNo;
    int arrival_time;
    int start_time;
    int end_time;
};

std::queue<ioReq*> inputQ;
std::vector<ioReq*> storeQ;
std::vector<ioReq*> waitlistQ;


class IOSche{
public:
    virtual ioReq* strategy() = 0;
    void changeDirection(ioReq *activeIO){
        if(activeIO->trackNo > head){
            direction = 1;
        }
        else{
            direction = -1;
        }
    }
};
IOSche *ioScheduer;

class FIFO: public IOSche{
public:
    ioReq* strategy()
    {
        ioReq *activeIO = waitlistQ.front();
        changeDirection(activeIO);
        waitlistQ.erase(waitlistQ.begin());
        return activeIO;
    }
};
class SSTF: public IOSche{
public:
    ioReq* strategy()
    {
        ioReq *activeIO = waitlistQ.front();
        int closest = 0;
        changeDirection(activeIO);
        for(int i = 0; i<waitlistQ.size();i++){
            if(abs(waitlistQ[i]->trackNo-head)<abs(activeIO->trackNo-head)){
                closest = i;
                activeIO = waitlistQ[i];
                changeDirection(activeIO);
            }
        }
        waitlistQ.erase(waitlistQ.begin()+closest);
        return activeIO;
    }
};

class LOOK: public IOSche{
public:
    ioReq* strategy()
    {
        ioReq *activeIO = nullptr;
        int closest = 0;
        int next_step = INT_MAX;
        for(int i = 0; i < waitlistQ.size();i++){
            if(direction*(waitlistQ[i]->trackNo-head)>=0 && next_step > abs(waitlistQ[i]->trackNo-head)){
                closest = i;
                next_step = abs(waitlistQ[i]->trackNo-head);
                activeIO = waitlistQ[i];
            }
        }
        if(activeIO){
            waitlistQ.erase(waitlistQ.begin()+closest);
            return activeIO;
        }
        else{
            direction = -direction;
            return activeIO = strategy();
        }

    }
};
//跳到最远再向开始走
class CLOOK: public IOSche{
public:
    ioReq* strategy()
    {
        ioReq *activeIO = nullptr;
        ioReq *start = nullptr;
        int farest=0;
        int closest = 0;
        int next_step = INT_MAX;
        int start_step = INT_MAX;
        if(direction == 1){
            for(int i = 0; i< waitlistQ.size();i++){
                if(start_step>waitlistQ[i]->trackNo){
                    start_step = waitlistQ[i]->trackNo;
                    farest = i;
                    start = waitlistQ[i];
                }
                if(waitlistQ[i]->trackNo-head>=0 && next_step > abs(waitlistQ[i]->trackNo-head)){
                    closest = i;
                    next_step = abs(waitlistQ[i]->trackNo-head);
                    activeIO = waitlistQ[i];
                }
            }
            if(!activeIO){
                direction = -1;
                activeIO = start;
                closest = farest;
            }
            waitlistQ.erase(waitlistQ.begin()+closest);
        }
        else{
            direction = 1;
            activeIO = strategy();
        }
        return activeIO;
    }
};

//2queue
std::vector<ioReq*> activeQ;
class FLOOK: public IOSche{
public:
    ioReq* strategy()
    {
        if(activeQ.empty()){
            activeQ.swap(waitlistQ);
        }
        ioReq *activeIO = nullptr;
        int closest = 0;
        int next_step = INT_MAX;
        for(int i = 0; i<activeQ.size();i++){
            if(direction*(activeQ[i]->trackNo-head)>=0 && next_step > abs(activeQ[i]->trackNo-head)){
                closest = i;
                next_step = abs(activeQ[i]->trackNo-head);
                activeIO = activeQ[i];
            }
        }
        if(activeIO){
            activeQ.erase(activeQ.begin()+closest);
            return activeIO;
        }
        if(!activeQ.empty()){
            direction = -direction;
            activeIO = strategy();
        }
        return activeIO;

    }
};


int main(int argc, char ** argv) {

//读取

    char* algo = NULL;
    if(getopt(argc, argv, "s:") != -1){
        algo = optarg;
    }
    if (*algo == 'i')  ioScheduer = new FIFO();
    else if (*algo == 'j') ioScheduer = new SSTF();
    else if (*algo == 's') ioScheduer = new LOOK();
    else if (*algo == 'c') ioScheduer = new CLOOK();
    else if (*algo == 'f') ioScheduer = new FLOOK();
    else printf("not available algo");

    std::ifstream inputfile(argv[optind]);
    std::string line;
    int reqNo = 0;
    while(getline(inputfile,line)){
        if(line[0]!='#'){
            ioReq* request = new ioReq;
            std::istringstream iss(line);
            request->ioNo = reqNo;
            iss >> request->arrival_time;
            iss >> request->trackNo;
            inputQ.push(request);
            storeQ.push_back(request);
            reqNo++;
        }
    }
    //stat
    int total_time = 0;
    int tot_movement = 0;
    int max_waittime = 0;
    double avg_turnaround = 0, avg_waittime=0;
    ioReq *newIO = nullptr;
    
//simulation
    while (!inputQ.empty()||!waitlistQ.empty()||newIO){
//        if a new I/O arrived to the system at this current time → add request to IO-queue
        if(!inputQ.empty() && inputQ.front()->arrival_time == total_time){
            waitlistQ.push_back(inputQ.front());
            inputQ.pop();
        }
//        if an IO is active and completed at this time → Compute relevant info and store in IO request for final summary
//        if an IO is active → Move the head by one unit in the direction its going (to simulate seek)
        if(newIO){
            if(newIO->trackNo == head){
                newIO->end_time = total_time;
                newIO = nullptr;
            }
            else{
                head += direction;
                tot_movement++;
            }
        }

//        if no IO request active now
//            if requests are pending → Fetch the next request from IO-queue and start the new IO.
//            else if all IO from input file processed → exit simulation
            while(!newIO &&!(activeQ.empty()&&waitlistQ.empty())){

                newIO = ioScheduer->strategy();
                if(newIO){
                    newIO->start_time = total_time;
                    if(newIO->trackNo == head){
                        newIO->end_time = total_time;
                        newIO = nullptr;
                    }
                    else{
                        head += direction;
                        tot_movement++;
                    }
                }
            }
        total_time++;

//        Increment time by 1

    }


//        if no IO request active now
//            if requests are pending → Fetch the next request from IO-queue and start the new IO.
//            else if all IO from input file processed → exit simulation
    //not ok for f still need new IO
//            while(!activeIO &&!(activeQ.empty()&&waitlistQ.empty())){
//
//                ioScheduer->strategy();
//                if(activeIO){
//                    activeIO->start_time = total_time;
//                    if(activeIO->trackNo == head){
//                        activeIO->end_time = total_time;
//                        activeIO = nullptr;
//                    }
//                    else{
//                        head += direction;
//                        tot_movement++;
//                    }
//                }
//            }

//待打印部分
    for(int i = 0; i<reqNo;i++){
        printf("%5d: %5d %5d %5d\n",i, storeQ[i]->arrival_time, storeQ[i]->start_time, storeQ[i]->end_time);
        avg_turnaround += storeQ[i]->end_time - storeQ[i]->arrival_time;
        avg_waittime += storeQ[i]->start_time - storeQ[i]->arrival_time;
        max_waittime = max(max_waittime,storeQ[i]->start_time - storeQ[i]->arrival_time);
    }
    avg_turnaround /= reqNo;
    avg_waittime /= reqNo;

//    printf("%5d: %5d %5d %5d\n",i, req->arrival_time, r->start_time, r->end_time);
    printf("SUM: %d %d %.2lf %.2lf %d\n", total_time-1, tot_movement, avg_turnaround, avg_waittime, max_waittime);
    delete ioScheduer;
}
