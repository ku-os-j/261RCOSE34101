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

const char* ALGO_NAMES[] = {
	"FCFS",
    "SJF (Non-preemptive)",
    "Priority (Non-preemptive)",
    "Round Robin",
    "Preemptive SJF",
    "Preemptive Priority"
};

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

// - Schedule: 메인 시뮬레이션 루프 및 알고리즘 구현
void Schedule(int algo_type) {
    printf("\n* %s\n\n|", ALGO_NAMES[algo_type]);
    
    int current_time = 0;
    int completed_processes = 0;
    
    int running_pid = -1; // 현재 CPU에서 실행 중인 프로세스 인덱스 (-1은 CPU가 비어있음을 의미)
	int prev_running_pid = -1;

    // 모든 프로세스가 완료될 때까지 1단위 시간(Tick)씩 흘려보내는 루프
    while(completed_processes < MAX_PROCESS) {
        
        // 1. 현재 시간(current_time)에 도착한 프로세스가 있다면 Job Queue -> Ready Queue로 이동
		for (int i = 0; i < MAX_PROCESS; i++) {
            if (job_queue[i].arrival_time == current_time) {
                // 레디큐의 맨 뒤(ready_count 위치)에 해당 프로세스의 인덱스를 저장
                ready_queue[ready_count] = i; 
                ready_count++;
            }
        }
				        
        // 2. I/O burst가 있다면 Waiting Queue로
        for (int i = 0; i < waiting_count; i++) {
            int p_idx = waiting_queue[i];
            remaining_io[p_idx]--; // 휴식 시간 1초 감소
            
            if (remaining_io[p_idx] == 0) {
                // 다 쉬었으면 Ready Queue로 복귀
                ready_queue[ready_count] = p_idx;
                ready_count++;
                
                // Waiting Queue에서 제거 및 메우기
                for (int j = i; j < waiting_count - 1; j++) {
                    waiting_queue[j] = waiting_queue[j + 1];
                }
                waiting_count--;
                i--; // 배열을 메우면서 바뀐 인덱스로 검사 계속
            }
        }
		
        // 시간 흐름
        current_time++;
        
        // 무한루프 방지
        if(current_time > 200) {
            printf("Error: Infinite loop suspected!\n");
            break;
        }
    }
printf("|\n00");
for (int k = 0; k < current_time; k++)
	printf("   %02d", k+1);
}
