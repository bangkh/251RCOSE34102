#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESSES 10
#define TIME_QUANTUM 2
#define MAX_IO 3
#define MAX_TIME 1000

typedef struct {
    int time;
    int burst;
    int done;
} IOEvent;

typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;
    int start_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;
    int completed;
    int io_event_cnt;
    IOEvent io_events[MAX_IO];
    int in_io;
    int io_progress;
    int io_remain;
    int executed;
} Process;

Process processes[MAX_PROCESSES];

typedef struct {
    int start, end, pid;
} GanttBlock;

GanttBlock gantt[1000];
int gantt_cnt = 0;

void Record_Gantt(int pid, int time) {
    if (gantt_cnt == 0 || gantt[gantt_cnt-1].pid != pid) { //이전 프로세스와 지금 프로세스가 다르다면
        if (gantt_cnt > 0) gantt[gantt_cnt-1].end = time;//이전 프로세스의 끝나는 시간을 기록
        gantt[gantt_cnt++] = (GanttBlock){time, -1, pid};//새로운 프로세스 시작, 나중에 덮어쓰기, 실행 중인 pid
    }
}

void Generate_IO_Events(int i) {//IO 만들자
    processes[i].io_event_cnt = rand() % 2; //프로세스 별로 1개 또는 0개 이니, IO는 process 개수 만큼 생성가능
    int prev = 1;//IO 이전과 다음을 배치하기 위한 기준값 설정
    for (int j = 0; j < processes[i].io_event_cnt; j++) {//IO생성 j가 IO 발생횟수
        int req = prev + rand() % ((processes[i].burst_time - prev - (processes[i].io_event_cnt - j)) + 1);
        //IO 발생 시점 설정, 기준값에 (남은 범위 - 실제로 남은 IO 횟수 + 1)을 해서 랜덤으로 위치 설정
        int dur = (rand() % 3) + 1; //IO 소요시간
        processes[i].io_events[j] = (IOEvent){req, dur, 0};//IO의 발생 위치, 소요시간, 처리 완료여부(0이면 미완)
        prev = req + 1;//다음 IO 위치 -> 기준값 변경
    }
}

void Create_Process(int n) {//process에 들어가는 변수들 정의의
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        processes[i].arrival_time = rand() % 10;
        processes[i].burst_time = (rand() % 8) + 6;
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].priority = rand() % 5 + 1;
        processes[i].start_time = -1;
        processes[i].finish_time = 0;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].completed = 0;
        processes[i].executed = 0;
        processes[i].in_io = 0;
        processes[i].io_progress = 0;
        processes[i].io_remain = 0;
        Generate_IO_Events(i);
    }
}

void Config(int n) {//초기화함수수
    gantt_cnt = 0;
    for (int i = 0; i < n; i++) {
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].completed = 0;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].start_time = -1;
        processes[i].finish_time = 0;
        processes[i].executed = 0;
        processes[i].in_io = 0;
        processes[i].io_progress = 0;
        processes[i].io_remain = 0;
        for (int j = 0; j < processes[i].io_event_cnt; j++)
            processes[i].io_events[j].done = 0;
    }
}

void Schedule_FCFS(int n) {
    int time = 0, completed = 0, running = -1;//초기값
    printf("\n=== Gantt Chart ===\n");
    while (completed < n) {//끝날때까지
        for (int i = 0; i < n; i++) {//IO 확인해보자
            if (processes[i].in_io) {//IO 있으면, in_io는 IO 작업 중인지 확인하는 플래그그
                if (--processes[i].io_remain == 0) {//IO 1줄어든 것이 0-> IO 끝나면
                    processes[i].in_io = 0;//다시 원래 위치로 복귀
                    processes[i].io_progress++;//다음 IO로 이동동
                }
            }
        }

        if (running == -1 || processes[running].in_io || processes[running].completed) {
            //진행하는 process가 없거나 IO 중이거나, process가 끝났을때
            int earliest = 1e9;//최대값 상수 
            for (int i = 0; i < n; i++) {
                if (!processes[i].completed && !processes[i].in_io && processes[i].arrival_time <= time) {
                    //종료도 아니고 IO도 아니고 이미 도착했을 때때
                    if (processes[i].arrival_time < earliest) {
                        earliest = processes[i].arrival_time;//earliest 값에 저장하고 도착한 다른 프로세스와 비교
                        running = i;//진행될 프로세스 선택택
                    }
                }
            }
        }

        int curr_pid = (running != -1) ? processes[running].pid : 0;
        //진행 중인 프로세스 있으면 그 process pid 없으면 0(idle)
        Record_Gantt(curr_pid, time);

        if (running != -1) {//process 진행중이면
            if (processes[running].start_time == -1) //초기값이면(한번도 실행된적없다면)
                processes[running].start_time = time; //시작시간 적기

            if (!processes[running].in_io){
            processes[running].remaining_time--;//남은 시간 줄이기
            processes[running].executed++;//실행시간 누적
            }

            if (processes[running].io_progress < processes[running].io_event_cnt) {
                //진행중인 IO와 남은 IO 비교로 IO가 남았다면
                IOEvent* ev = &processes[running].io_events[processes[running].io_progress];
                //다음 IO 이벤트를 포인터로 가르킴킴
                if (!ev->done && processes[running].executed == ev->time) {
                    //IO가 끝난적 없고, Process 실행된 시간이 IO 요청 시점과 같다면 
                    processes[running].in_io = 1;//IO 카운터 1로 올림(IO 진행)
                    processes[running].io_remain = ev->burst;//IO 얼마나 진행하는지 표시시
                    ev->done = 1;//이번 IO 처리 완료
                }
            }

            if (processes[running].remaining_time == 0) {//프로세스가 끝나면
                processes[running].finish_time = time + 1;//프로세스 끝난 시간 표시시
                processes[running].turnaround_time = processes[running].finish_time - processes[running].arrival_time;
                //턴어라운드 타임
                processes[running].waiting_time = processes[running].turnaround_time - processes[running].burst_time;
                //웨이팅타임
                processes[running].completed = 1;
                //프로세스 끝남
                completed++;
                running = -1;
                //running queue 비었음
            }
        }
        time++;//시간 지남
    }
    gantt[gantt_cnt-1].end = time;//끝나는 시간 함수에 저장
    for (int i = 0; i < gantt_cnt; i++) {//간트 함수 쓰기기
        if (gantt[i].pid == 0) printf("| Idle ");//간트 pid가 0-> 프로세스가 아님
        else if(gantt[i].pid < 0) printf("| IO ");
        else printf("| P%d ", gantt[i].pid);//간트 차트에 프로세스 pid
    }
    printf("|\n");
    for (int i = 0; i < gantt_cnt; i++) printf("%5d ", gantt[i].start);//각 블록에 시작시간
    printf("%5d\n", gantt[gantt_cnt-1].end);//끝나는 시간 출력
}

void Print_Processes(int n) {//프로세스 정보 출력
    printf("\nPID\tArrival\tBurst\tStart\tFinish\tWaiting\tTurnaround\tI/O Events\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t", processes[i].pid, processes[i].arrival_time, processes[i].burst_time,
               processes[i].start_time, processes[i].finish_time,
               processes[i].waiting_time, processes[i].turnaround_time);
        for (int j = 0; j < processes[i].io_event_cnt; j++) {
            printf(" [at %d, %ds]", processes[i].io_events[j].time, processes[i].io_events[j].burst);
        }
        printf("\n");
    }
}

void Schedule_NonPreemptiveSJF(int n) {
    int time = 0, completed = 0, running = -1;
    printf("\n=== Non-Preemptive SJF Gantt Chart ===\n");

    while (completed < n) {//끝날때까지
        for (int i = 0; i < n; i++) {//IO 확인해보자
            if (processes[i].in_io) {//IO 있으면, in_io는 IO 작업 중인지 확인하는 플래그그
                if (--processes[i].io_remain == 0) {//IO 1줄어든 것이 0-> IO 끝나면
                    processes[i].in_io = 0;//다시 원래 위치로 복귀
                    processes[i].io_progress++;//다음 IO로 이동동
                }
            }
        }

        // 프로세스 선택
        if (running == -1 || processes[running].in_io || processes[running].completed) {
            int shortest = 1e9;
            running = -1;
            for (int i = 0; i < n; i++) {
                if (!processes[i].completed && !processes[i].in_io && processes[i].arrival_time <= time) {
                    if (processes[i].remaining_time < shortest) {
                        shortest = processes[i].remaining_time;
                        running = i;
                    }
                }
            }
        }

        int curr_pid = (running != -1) ? processes[running].pid : 0;
        Record_Gantt(curr_pid, time);

        if (running != -1) {
            if (processes[running].start_time == -1)
                processes[running].start_time = time;//프로세스 시작 시간 적기기

            processes[running].remaining_time--;
            processes[running].executed++;//실행시간 누적

            // I/O 요청 발생
            if (processes[running].io_progress < processes[running].io_event_cnt) {
                //진행중인 IO와 남은 IO 비교로 IO가 남았다면
                IOEvent* ev = &processes[running].io_events[processes[running].io_progress];
                //다음 IO 이벤트를 포인터로 가르킴킴
                if (!ev->done && processes[running].executed == ev->time) {
                    //IO가 끝난적 없고, Process 실행된 시간이 IO 요청 시점과 같다면 
                    processes[running].in_io = 1;//IO 카운터 1로 올림(IO 진행)
                    processes[running].io_remain = ev->burst;//IO 얼마나 진행하는지 표시
                    ev->done = 1;//이번 IO 처리 완료
                }
            }
            if (processes[running].remaining_time == 0) {//프로세스가 끝나면
                processes[running].finish_time = time + 1;//프로세스 끝난 시간 시시
                processes[running].turnaround_time = processes[running].finish_time - processes[running].arrival_time;
                //턴어라운드 타임
                processes[running].waiting_time = processes[running].turnaround_time - processes[running].burst_time;
                //웨이팅타임
                processes[running].completed = 1;
                //프로세스 끝남
                completed++;
                running = -1;
                //running queue 비었음
            }
        }

        time++;
    }

    gantt[gantt_cnt - 1].end = time;

    for (int i = 0; i < gantt_cnt; i++)
        printf("| %s%d ", gantt[i].pid == 0 ? "Idle" : "P", gantt[i].pid);
    printf("|\n");

    for (int i = 0; i < gantt_cnt; i++)
        printf("%5d ", gantt[i].start);
    printf("%5d\n", gantt[gantt_cnt - 1].end);
}
void Schedule_PreemptiveSJF(int n) {
    int time = 0, completed = 0, running = -1;
    printf("\n=== Preemptive SJF Gantt Chart ===\n");

    while (completed < n) {
        // I/O 상태 처리
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io && --processes[i].io_remain == 0) {
                processes[i].in_io = 0;
                processes[i].io_progress++;
            }
        }

        // 가장 짧은 remaining_time을 가진 프로세스 선택
        int shortest = 1e9;//위의 알고리즘과 다른 것은 매 시간마다 짧은 시간을 갖고 있는 프로세스를 확인함
        int next = -1;
        for (int i = 0; i < n; i++) {
            if (!processes[i].completed && !processes[i].in_io && processes[i].arrival_time <= time) {
                if (processes[i].remaining_time < shortest) {
                    shortest = processes[i].remaining_time;
                    next = i;
                }
            }
        }

        running = next;

        int curr_pid = (running != -1) ? processes[running].pid : 0;
        Record_Gantt(curr_pid, time);

        if (running != -1) {
            if (processes[running].start_time == -1)
                processes[running].start_time = time;

            processes[running].remaining_time--;
            processes[running].executed++;

            // I/O 발생 조건 체크
            if (processes[running].io_progress < processes[running].io_event_cnt) {
                IOEvent* ev = &processes[running].io_events[processes[running].io_progress];
                if (!ev->done && processes[running].executed == ev->time) {
                    processes[running].in_io = 1;
                    processes[running].io_remain = ev->burst;
                    ev->done = 1;
                }
            }

            if (processes[running].remaining_time == 0 && !processes[running].in_io) {
                processes[running].finish_time = time + 1;
                processes[running].turnaround_time = processes[running].finish_time - processes[running].arrival_time;
                processes[running].waiting_time = processes[running].turnaround_time - processes[running].burst_time;
                processes[running].completed = 1;
                completed++;
                running = -1;
            }
        }

        time++;
    }

    gantt[gantt_cnt - 1].end = time;

    for (int i = 0; i < gantt_cnt; i++)
        printf("| %s%d ", gantt[i].pid == 0 ? "Idle" : "P", gantt[i].pid);
    printf("|\n");

    for (int i = 0; i < gantt_cnt; i++)
        printf("%5d ", gantt[i].start);
    printf("%5d\n", gantt[gantt_cnt - 1].end);
}

void Schedule_NonPreemptivePriority(int n) {
    int time = 0, completed = 0, running = -1;
    printf("\n=== Non-Preemptive Priority Gantt Chart ===\n");

    while (completed < n) {//끝날때까지
        for (int i = 0; i < n; i++) {//IO 확인해보자
            if (processes[i].in_io) {//IO 있으면, in_io는 IO 작업 중인지 확인하는 플래그그
                if (--processes[i].io_remain == 0) {//IO 1줄어든 것이 0-> IO 끝나면
                    processes[i].in_io = 0;//다시 원래 위치로 복귀
                    processes[i].io_progress++;//다음 IO로 이동동
                }
            }
        }

        // 새 프로세스 선택
        if (running == -1 || processes[running].in_io || processes[running].completed) {
            int best = 1e9;
            running = -1;
            for (int i = 0; i < n; i++) {
                if (!processes[i].completed && !processes[i].in_io && processes[i].arrival_time <= time) {
                    if (processes[i].priority < best) {
                        best = processes[i].priority;
                        running = i;
                    }
                }
            }
        }

        int curr_pid = (running != -1) ? processes[running].pid : 0;
        Record_Gantt(curr_pid, time);

        if (running != -1) {
            if (processes[running].start_time == -1)
                processes[running].start_time = time;

            processes[running].remaining_time--;
            processes[running].executed++;

            // I/O 인터럽트 발생 조건
            if (processes[running].io_progress < processes[running].io_event_cnt) {
                IOEvent* ev = &processes[running].io_events[processes[running].io_progress];
                if (!ev->done && processes[running].executed == ev->time) {
                    processes[running].in_io = 1;
                    processes[running].io_remain = ev->burst;
                    ev->done = 1;
                }
            }

            if (processes[running].remaining_time == 0 && !processes[running].in_io) {
                processes[running].finish_time = time + 1;
                processes[running].turnaround_time = processes[running].finish_time - processes[running].arrival_time;
                processes[running].waiting_time = processes[running].turnaround_time - processes[running].burst_time;
                processes[running].completed = 1;
                completed++;
                running = -1;
            }
        }

        time++;
    }

    gantt[gantt_cnt - 1].end = time;

    for (int i = 0; i < gantt_cnt; i++)
        printf("| %s%d ", gantt[i].pid == 0 ? "Idle" : "P", gantt[i].pid);
    printf("|\n");

    for (int i = 0; i < gantt_cnt; i++)
        printf("%5d ", gantt[i].start);
    printf("%5d\n", gantt[gantt_cnt - 1].end);
}

void Schedule_PreemptivePriority(int n) {
    int time = 0, completed = 0, running = -1;
    printf("\n=== Preemptive Priority Gantt Chart ===\n");

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                if (--processes[i].io_remain == 0) {
                    processes[i].in_io = 0;
                    processes[i].io_progress++;
                }
            }
        }

        int best = 1e9, next = -1;
        for (int i = 0; i < n; i++) {
            if (!processes[i].completed && !processes[i].in_io && processes[i].arrival_time <= time) {
                if (processes[i].priority < best) {
                    best = processes[i].priority;
                    next = i;
                }
            }
        }

        running = next;

        if (running != -1) {
            Record_Gantt(processes[running].pid, time);
            if (processes[running].start_time == -1)
                processes[running].start_time = time;

            processes[running].remaining_time--;
            processes[running].executed++;

            if (processes[running].io_progress < processes[running].io_event_cnt) {
                IOEvent* ev = &processes[running].io_events[processes[running].io_progress];
                if (!ev->done && processes[running].executed == ev->time) {
                    ev->done = 1;
                    processes[running].in_io = 1;
                    processes[running].io_remain = ev->burst;
                    if (gantt_cnt > 0 && gantt[gantt_cnt - 1].end == -1)
                        gantt[gantt_cnt - 1].end = time + 1;
                }
            }

            if (processes[running].remaining_time == 0 && !processes[running].in_io) {
                processes[running].finish_time = time + 1;
                processes[running].turnaround_time = processes[running].finish_time - processes[running].arrival_time;
                processes[running].waiting_time = processes[running].turnaround_time - processes[running].burst_time;
                processes[running].completed = 1;
                completed++;
                if (gantt_cnt > 0 && gantt[gantt_cnt - 1].end == -1)
                    gantt[gantt_cnt - 1].end = time + 1;
                running = -1;
            }
        } else {
            Record_Gantt(0, time);
        }

        time++;
    }

    if (gantt_cnt > 0 && gantt[gantt_cnt - 1].end == -1)
        gantt[gantt_cnt - 1].end = time;

    for (int i = 0; i < gantt_cnt; i++)
        printf("| %s%d ", gantt[i].pid == 0 ? "Idle" : "P", gantt[i].pid);
    printf("|\n");
    for (int i = 0; i < gantt_cnt; i++)
        printf("%5d ", gantt[i].start);
    printf("%5d\n", gantt[gantt_cnt - 1].end);
}

#define QUEUE_SIZE (MAX_PROCESSES * MAX_TIME)

void Schedule_RoundRobin(int n, int quantum) {
    int time = 0, completed = 0;
    int queue[QUEUE_SIZE], front = 0, rear = 0;
    int visited[MAX_PROCESSES] = {0};

    printf("\n=== Round Robin Gantt Chart ===\n");

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                if (--processes[i].io_remain == 0) {
                    processes[i].in_io = 0;
                    processes[i].io_progress++;
                    queue[rear] = i;
                    rear = (rear + 1) % QUEUE_SIZE;
                }
            }
        }

        for (int i = 0; i < n; i++) {
            if (!visited[i] && processes[i].arrival_time <= time) {
                queue[rear] = i;
                rear = (rear + 1) % QUEUE_SIZE;
                visited[i] = 1;
            }
        }

        if (front == rear) {
            Record_Gantt(0, time);
            time++;
            continue;
        }

        int idx = queue[front];
        front = (front + 1) % QUEUE_SIZE;

        if (processes[idx].completed || processes[idx].in_io)
            continue;

        if (processes[idx].start_time == -1)
            processes[idx].start_time = time;

        int exec = (processes[idx].remaining_time > quantum) ? quantum : processes[idx].remaining_time;

        for (int t = 0; t < exec; t++) {
            Record_Gantt(processes[idx].pid, time);
            time++;
            processes[idx].remaining_time--;
            processes[idx].executed++;

            for (int i = 0; i < n; i++) {
                if (!visited[i] && processes[i].arrival_time <= time) {
                    queue[rear] = i;
                    rear = (rear + 1) % QUEUE_SIZE;
                    visited[i] = 1;
                }
            }

            if (processes[idx].io_progress < processes[idx].io_event_cnt) {
                IOEvent* ev = &processes[idx].io_events[processes[idx].io_progress];
                if (!ev->done && processes[idx].executed == ev->time) {
                    processes[idx].in_io = 1;
                    processes[idx].io_remain = ev->burst;
                    ev->done = 1;
                    if (gantt_cnt > 0 && gantt[gantt_cnt - 1].end == -1)
                        gantt[gantt_cnt - 1].end = time;
                    break;
                }
            }

            if (processes[idx].remaining_time == 0) {
                processes[idx].finish_time = time;
                processes[idx].turnaround_time = time - processes[idx].arrival_time;
                processes[idx].waiting_time = processes[idx].turnaround_time - processes[idx].burst_time;
                processes[idx].completed = 1;
                completed++;
                if (gantt_cnt > 0 && gantt[gantt_cnt - 1].end == -1)
                    gantt[gantt_cnt - 1].end = time;
                break;
            }
        }

        if (!processes[idx].completed && !processes[idx].in_io) {
            queue[rear] = idx;
            rear = (rear + 1) % QUEUE_SIZE;
        }
    }

    if (gantt_cnt > 0 && gantt[gantt_cnt - 1].end == -1)
        gantt[gantt_cnt - 1].end = time;

    for (int i = 0; i < gantt_cnt; i++)
        printf("| %s%d ", gantt[i].pid == 0 ? "Idle" : "P", gantt[i].pid);
    printf("|\n");
    for (int i = 0; i < gantt_cnt; i++)
        printf("%5d ", gantt[i].start);
    printf("%5d\n", gantt[gantt_cnt - 1].end);
}


int main() {
    int n;

    printf("Enter the number of processes (1 to %d): ", MAX_PROCESSES);
    scanf("%d", &n);

    if (n <= 0 || n > MAX_PROCESSES) {
        printf("Invalid number of processes. Please enter a value between 1 and %d.\n", MAX_PROCESSES);
        return -1;
    }
    Create_Process(n);

    printf("\n--- FCFS ---\n");
    Config(n);
    Schedule_FCFS(n);
    Print_Processes(n);

    printf("\n--- NonPreemptive SJF ---\n");
    Config(n);
    Schedule_NonPreemptiveSJF(n);
    Print_Processes(n);

    printf("\n--- Preemptive SJF ---\n");
    Config(n);
    Schedule_PreemptiveSJF(n);
    Print_Processes(n);

    printf("\n--- NonPreemptive Priority ---\n");
    Config(n);
    Schedule_NonPreemptivePriority(n);
    Print_Processes(n);

    printf("\n--- Preemptive Priority ---\n");
    Config(n);
    Schedule_PreemptivePriority(n);
    Print_Processes(n);

    printf("\n--- Round Robin ---\n");
    Config(n);
    Schedule_RoundRobin(n, TIME_QUANTUM);
    Print_Processes(n);

    return 0;
}
