#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESS 5

// 프로세스 정보 구조체 정의
typedef struct {
    int pid;
    int arrival_time;
    int cpu_burst;
    int io_burst;
	int io_count;
    int io_triggers[2];		// 한 프로세스가 CPU를 쓰는 동안 I/O 요청은 최대 2번
	int priority;
	
// 평가(Evaluation)를 위한 변수들
    int remaining_cpu;
    int waiting_time;
    int turnaround_time;
    int completion_time;
} Process;

// 전역 변수
Process job_queue[MAX_PROCESS];

// 함수 프로토타입 선언
void Create_Process();
void Config();
void Schedule(int algo_type);
void Evaluation();

int main() {
    srand(time(NULL));
    
	printf("\n===== CPU Scheduling Simulator =====\n");

    Create_Process();
	
	for(int i = 0; i < 6; i++) {
    Config(); 
    Schedule(i);
    Evaluation();
	}
    
    return 0;
}