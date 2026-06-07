#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESS 5
#define RR_TIME_QUANTUM 2

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

	int time_quantum_counter = 0; // RR 알고리즘 전용 카운터

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
		
        // 3. 스케줄링 알고리즘(algo_type)에 따라 Ready Queue에서 프로세스 하나 선택하여 run
        //    0- FCFS: 먼저 온 프로세스
        //    1- SJF: remaining_cpu가 가장 짧은 프로세스
        //    2- Priority: 우선순위가 가장 높은 프로세스
        //    3- RR: Time Quantum 만큼 실행하고 교체
        //    4- Preemptive SJF: running 프로세스 포함 remaining_cpu가 가장 짧은 프로세스
        //    5- Preemptive Priority: running process 포함 우선순위가 가장 높은 프로세스   
		switch (algo_type) {
			case 0:// FCFS
                // CPU가 비어있고, 레디 큐에 대기 중인 프로세스가 있다면 맨 앞 프로세스를 run
                if (running_pid == -1 && ready_count > 0) {
                    running_pid = ready_queue[0];
                    
                    // 레디 큐 한 칸씩 당기기
                    for (int j = 0; j < ready_count - 1; j++) {
                        ready_queue[j] = ready_queue[j + 1];
                    }
                    ready_count--;
                }
                break;			
			
			case 3:// Round Robin
                // CPU가 비어있고, 레디 큐에 대기 중인 프로세스가 있다면 맨 앞 프로세스를 run
                if (running_pid == -1 && ready_count > 0) {
                    running_pid = ready_queue[0];
                    
                    // 레디 큐 한 칸씩 당기기
                    for (int j = 0; j < ready_count - 1; j++) {
                        ready_queue[j] = ready_queue[j + 1];
                    }
                    ready_count--;
                    
                    time_quantum_counter = 0; // 새로운 프로세스 run -> 카운터 리셋
                }
                // 이미 돌아가고 있는 프로세스가 타임 퀀텀을 다 채웠다면
                else if (running_pid != -1 && time_quantum_counter == RR_TIME_QUANTUM) {
                    
                    // 돌아가던 프로세스를 레디 큐의 맨 뒤로
                    ready_queue[ready_count] = running_pid;
                    ready_count++;
                    
                    // 레디 큐의 맨 앞을 run
                    running_pid = ready_queue[0];
                    for (int j = 0; j < ready_count - 1; j++) {
                        ready_queue[j] = ready_queue[j + 1];
                    }
                    ready_count--;
                    
                    time_quantum_counter = 0; // 카운터 리셋
                }
                break;
						
			default:
				break;
		}
		
        // 4. 선택된 프로세스 실행 -> remaining_cpu 감소
        //    	만약 remaining_cpu == 0 되면 프로세스 종료 이후 completed_processes 증가
        //    	종료 시점의 current_time을 이용해 Turnaround time, Waiting time 계산
        if (running_pid != -1) {
            job_queue[running_pid].remaining_cpu--;			// running 프로세스 remaining_cpu를 1초 감소
            
			if (algo_type == 3) time_quantum_counter++; 	// RR 알고리즘일 경우 타임 퀀텀 카운터를 1초 증가
			
			printf("| %d |", job_queue[running_pid].pid);	// Gantt Chart (process running)

            // 일하다가 중간에 I/O 트리거를 만났는지 검사
            int execution_progress = job_queue[running_pid].cpu_burst - job_queue[running_pid].remaining_cpu;
            int triggered_io = 0;
            for (int k = 0; k < job_queue[running_pid].io_count; k++) {
                if (job_queue[running_pid].io_triggers[k] == execution_progress && job_queue[running_pid].remaining_cpu > 0) {
                    // 트리거 발동 Waiting Queue로 이동
                    waiting_queue[waiting_count] = running_pid;
                    waiting_count++;
                    remaining_io[running_pid] = job_queue[running_pid].io_burst; // remaining_io(waiting 시간) 세팅
                    
                    running_pid = -1; // CPU 비우기
                    triggered_io = 1;
                    
					if (algo_type == 3) time_quantum_counter = 0; // I/O 때문에 쫓겨났으니 RR 카운터 리셋
					
					break;
                }
            }
		
		if (triggered_io == 0 && job_queue[running_pid].remaining_cpu == 0) { 			// 프로세스 완료

                // 종료 시점(Completion Time)
                job_queue[running_pid].completion_time = current_time + 1; 
                
                // 반환 시간(Turnaround Time) = 종료 시간 - 도착 시간
                job_queue[running_pid].turnaround_time = job_queue[running_pid].completion_time - job_queue[running_pid].arrival_time;
                
                running_pid = -1; // 다 썼으니 CPU 비우기
                completed_processes++; // 완료된 프로세스 개수 증가 (while문 탈출 조건)
            }
        } // if (running_pid != -1) 의 닫는 중괄호
        
        else {					//(running_pid == -1) 대기 큐나 레디 큐에만 프로세스들이 있을 경우 (Idle 상태)
			printf("| - |");	// Gantt Chart (idle)
        }
		
		// 대기 시간(Waiting Time)
		for (int q = 0; q < ready_count; q++) {
            int p_idx = ready_queue[q];
            job_queue[p_idx].waiting_time++;
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
