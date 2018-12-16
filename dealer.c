#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_CLIENT 5

typedef struct{		//메세지큐를 통해 데이터 주고받을 구조체
	long data_type;
	int pid;
	int value;
}q_data;

typedef struct p_result{	//쓰레드로부터 결과값을 전달받을 구조체
	int pid;
	int value;
	int client_count;
}P_RES;

int to_id;		//메세지큐 id값 변수
int from_id;

int running = 0;	//쓰레드 동작변수

int clear_q(int a);
void *thread_func(void *);
void sig_handler(int signo);
void main_sighandler(int signo);
int get_mid_value(int *arr, int max_client);

int main(){
	pthread_t pthread_t; // thread 변수
	q_data winner_data; // 각 클라이언트들에게 승자 정보를 전달하기 위한 변수
	P_RES *buf; // 생성된 thread로부터 해당 변수를 통해 값을 전달받는 변수

	int i, result, turn=1;

	from_id = msgget((key_t)8766, IPC_CREAT|0666); //값을 전달받기위한 메시지큐 생성
	if(from_id == -1 ){
		perror("msgget() 실패");
		exit(1);
	}

	to_id = msgget((key_t)8765, IPC_CREAT|0666); //값을 전달하기위한 메세지큐 생성
	if(to_id == -1){
		perror("msgget() 실패");
		exit(1);
	}

	signal(SIGINT, main_sighandler); //SIGINT시그널이 호출되면 main_sighandler 함수 호출

	while(1) {
		if(pthread_create(&pthread_t, NULL,thread_func, NULL)<0){
			perror("thread create error : ");
			exit(0);
		}

		printf("\n##########게임을 시작합니다[%d번째 턴]##########\n", turn);
		for(i=30; i>0; i--) {	//게임시간(30초) 를 출력해줌
			printf("%d\n",i);
			sleep(1);
		}
		printf("\n");
		pthread_kill(pthread_t, SIGUSR1); // 게임시간이 지나게되면 thread에게 SIGUSR1 전송
		pthread_join(pthread_t, (void*)&buf); //쓰레드가 종료되고 값을 반환될때까지 기다림

		printf("##################결과####################\n");
		printf("승자Pid : %d, 값 : %d, 참여유저수 : %d\n", buf->pid, buf->value, buf->client_count);	//반환된 값을 출력함

		memset(&winner_data, 0, sizeof(q_data)); //winner_data구조체안의 변수들을 모두 초기화
		winner_data.data_type = 1;	//winner_data구조체 안에 반환값 입력
		winner_data.pid = buf->pid;
		winner_data.value = buf->value;

		for(i=0; i<buf->client_count; i++){ //참여한 플레이어수만큼 winner_data를 메세지큐에 전송
			result = msgsnd(to_id, &winner_data, sizeof(winner_data)-sizeof(long), 0);
			if (result == -1){
				perror("msgsnd failed :");
				exit(1);
			}
		}
		turn++;
		clear_q(to_id); //큐 초기화
	}
	return 0;
}


void *thread_func(void *arg){ //쓰레드 생성시에 동작될 함수
	int rcv;
	int cur_client = 0;
	int val_arr[MAX_CLIENT];
	int pid_arr[MAX_CLIENT];
	int idx;
	struct sigaction act;
	q_data msg;
	P_RES *result_msg;

	// 결과 값을 위한 구조체 메모리 할당
	//쓰레드 내부 로컬변수이기때문에 쓰레드 종료시 주소값이 유효하지 않으므로 쓰레드가 종료되고나서도 사용하기위해 동적할당
	//값을 메인함수의 buf 반환하기위해 사용
	result_msg = (P_RES*)malloc(sizeof(P_RES));

	// 쓰레드에 시그널 등록
	act.sa_handler = sig_handler;
	sigaction(SIGUSR1, &act, NULL);
	
	running = 1; // 1일때 스레드 동작 0일때 종료

	while(running){
		rcv = msgrcv(from_id, &msg, sizeof(q_data)-sizeof(long), 0, IPC_NOWAIT);//메세지큐로부터 데이터를 읽음
		if(rcv != -1 && cur_client < MAX_CLIENT){ //최대인원수보다 현재카운트된 플레이어가 적거나 데이터를 읽을때 반환값이 -1이 아니라면 값을 읽어서 배열에 저장
			printf("값 받음  [pid : %d 값 : %d]\n", msg.pid, msg.value);
			pid_arr[cur_client] = msg.pid;
			val_arr[cur_client] = msg.value;
			cur_client++;
		}
		else {
			sleep(1); //메시지큐에 데이터가없으면 sleep(1)을해주어서 위에있는 30초 시간과 맞춰준다.
		}
	}

	memset(result_msg, 0, sizeof(P_RES));  //중간값을 전송할 구조체 초기화
	if(cur_client > 2){ //3인이상일때만 게임이 진행되도록 하는 조건문
		idx = get_mid_value(val_arr, cur_client); //중간값이있는 배열의 인덱스정보를 get_mid_value 함수를 통해 전달받음
		result_msg->pid = pid_arr[idx]; //전달된 인덱스의 배열값들을 딜러프로세스에게 전달할 구조체에 입력
		result_msg->value = val_arr[idx];
		result_msg->client_count = cur_client;
	}
	else { //2인 이하일경우에 게임이 진행되지않으므로 값들을 초기화시킴
		result_msg->pid = 0;
		result_msg->value = 0;
		result_msg->client_count = cur_client; //cur_client변수는 현재 종료하지않고 메시지큐에 데이터를 보낸 프로세스를 카운트하므로 그대로 놔둔다
	}
	pthread_exit(result_msg); //쓰레드를 종료하고 딜러프로세스에게 result_msg구조체의 데이터를 전달
	free(result_msg);
}

int get_mid_value(int *arr, int max_client){
	int i;
	int empty, c, d;
	int mid_idx, sorted_mid_idx;
	int sorted_arr[max_client];
	
	memcpy(sorted_arr, arr, sizeof(sorted_arr)); //입력받은배열을 복사하여 정렬을 위한 배열 1개 원본 배열 1개를 생성

	for (c=0 ; c<max_client-1 ; c++){ //버블정렬로 복사한 배열을 정렬
		for (d=0 ; d<max_client-c-1 ; d++){
			if (sorted_arr[d] > sorted_arr[d+1]){
				empty = sorted_arr[d];
				sorted_arr[d] = sorted_arr[d+1];
				sorted_arr[d+1] = empty;
			}
		}
	}

	// 만약 참여한 클라이언트가 짝수인 경우 작은 값을 중간 값
	// 작은값을 중간값으로할떄 동일한 값을 보낸 두프로세스가 존재할때 먼저 보낸 피드가 앞의 인덱스를 차지하게되므로 작은값을 승리자로 함
	if(max_client % 2  == 0)
		sorted_mid_idx = (max_client / 2) - 1;
	else {
		max_client--;	//배열은 0번부터 시작하므로 1개의 카운트를 빼고 계산하면
		sorted_mid_idx = max_client / 2; // 3명일때 2/2=1 1번인덱스가 중간값, 5명일때 4/2=2 2번인덱스가 중간값
	}
	
	for(i=0; i<max_client+1; i++){ //정렬되지않은 원본배열에서 정렬된 배열에 있던 중간값이 맞으면
		if(arr[i] == sorted_arr[sorted_mid_idx]){ //원본배열의 인덱스값을 저장한다.
			mid_idx = i;
			break;
		}
	}
	return mid_idx; //원본배열의 인덱스값을 리턴
}

void main_sighandler(int signo){
	printf("딜러 프로세스 종료\n");
	exit(0);
}

void sig_handler(int signo){
	printf("쓰레드 SIGUSR1받음 : 쓰레드 종료\n");
	running = 0; //쓰레드 동작변수를 0으로 만들어서 쓰레드를 동작하지않게끔 설정
}

int clear_q(int a){
	q_data msg;
	int rcv = msgrcv(a, &msg, sizeof(q_data)-sizeof(long), 0, IPC_NOWAIT);
	if(rcv == -1) return 0;
}
