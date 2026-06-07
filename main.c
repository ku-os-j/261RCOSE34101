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

int ready_queue[MAX_PROCESS]; 		// ready 큐에 있는 프로세스들의 PID 저장
int ready_count;				  	// 현재 ready 큐에 있는 프로세스 수
	
int waiting_queue[MAX_PROCESS]; 	// I/O 대기 중인 프로세스 인덱스
int waiting_count;                  // 현재 waiting 큐에 있는 프로세스 수
int remaining_io[MAX_PROCESS];  	// 각 프로세스의 남은 I/O 시간

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

// - Create_Process: 랜덤 데이터로 프로세스 생성
void Create_Process() {
    for(int i = 0; i < MAX_PROCESS; i++) {
        job_queue[i].pid = i + 1;
        job_queue[i].arrival_time = rand() % 5;      	// 0 ~ 4 사이 랜덤
        job_queue[i].cpu_burst = (rand() % 5) + 1;   	// 1 ~ 5 사이 랜덤
        job_queue[i].io_burst = rand() % 5 + 1;      	// 1 ~ 5 사이 랜덤
        job_queue[i].priority = (rand() % 5) + 1;    	// 1 ~ 5 사이 랜덤
        
		// 각 프로세스의 모든 io_trigger 시점을 -1로 초기화
		for (int j = 0; j < 2; j++) {
		job_queue[i].io_triggers[j] = -1;
		}
		
		// 한 프로세스가 CPU를 쓰는 동안 I/O 요청은 최대 2번
		int max_io_count = job_queue[i].cpu_burst - 1;
        if (max_io_count > 2) max_io_count = 2;
        if (max_io_count < 0) max_io_count = 0;
		
		if (max_io_count == 0) {
            job_queue[i].io_count = 0;
        } else {
            job_queue[i].io_count = rand() % (max_io_count + 1); // 0과 max_io_count 사이 랜덤
		}
		
		for (int j = 0; j < job_queue[i].io_count; j++) {
			while (1) {
			// 1과 (cpu_burst - 1) 사이에서 후보 시점을 하나 뽑기
			int target = (rand() % (job_queue[i].cpu_burst - 1)) + 1;
        
			// target이 이전에 이미 뽑힌 시점이랑 겹치는지 검사
			int check = 0;
			for (int k = 0; k < j; k++) {
				if (job_queue[i].io_triggers[k] == target) {
					check = 1;
					break;
				}
			}
        
			// 안 겹칠 경우에만 배열에 저장하고 while문 탈출
			if (check == 0) {
				job_queue[i].io_triggers[j] = target;
				break;
				}
			}
		}
		
        job_queue[i].remaining_cpu = job_queue[i].cpu_burst;
        job_queue[i].waiting_time = 0;
        job_queue[i].turnaround_time = 0;
        
    }
}

// - Config: 알고리즘 시작 전 큐 및 시스템 초기화
void Config() {
	for(int i = 0; i < MAX_PROCESS; i++) {
		// 각 프로세스의 시간 정보 초기화
        job_queue[i].remaining_cpu = job_queue[i].cpu_burst;
        job_queue[i].waiting_time = 0;
        job_queue[i].turnaround_time = 0;
		job_queue[i].completion_time = 0;
		// Ready Queue와 Wating Queue 초기화
		ready_queue[i] = 0;
        waiting_queue[i] = 0;
        remaining_io[i] = 0;
    }
		ready_count = 0;
		waiting_count = 0;
}
