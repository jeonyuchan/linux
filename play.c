#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>

typedef struct{
	long data_type;
	int pid;
	int value;
}t_data;

int from_id;
int to_id;

void sig_handler(int signo){
	printf("게임을 이용해주셔서 감사합니다! 클라이언트 종료하겠습니다. \n");
	exit(0);
}

int main(){
	int msg;
	int input;

	t_data data, rcv_data;

	signal(SIGINT, sig_handler);

	while(1){
		to_id = msgget((key_t)8766, IPC_CREAT |0666); //메시지큐 생성
		if(to_id == -1){
			perror("msgget() 실패");
			exit(1);
		}

		printf("중간값이라고 생각되는 값을 입력해주세요: "); scanf("%d", &input);
		data.data_type = 1;//data_type의 경우 0보다 큰값만 전송되므로, 이번 프로젝트에서는 사용안하고 전송을하기위해 데이터타입만 1로 바꿔줌
		data.pid = getpid();//자신의 피드를 구조체에 입력
		data.value = input;//입력받은값을 메시지큐에 전송할 구조체에 입력

		msg = msgsnd(to_id, &data, sizeof(t_data)-sizeof(long), 0);
		if(msg == -1){
			perror("msgsnd() 실패");
			exit(1);
		}



		from_id = msgget((key_t)8765, IPC_CREAT | 0666);
		if(from_id == -1){
			perror("msgget() 실패");
			exit(1);
		}

		msg = msgrcv(from_id, &rcv_data, sizeof(t_data)-sizeof(long), 0, 0);//메세지큐로부터 데이터를 받음
		if(msg == -1){	
			perror("msgrcv() 실패");
		}


		if(rcv_data.pid == getpid()) //받은데이터의 피드값이 내데이터의 피드와 일치하면 승자
			printf("이번턴의 중간값찾기 승자입니다 중간값 ( %d )\n\n", rcv_data.value);
		else if(rcv_data.pid == 0) //피드값이 0이면 딜러쪽에서는 2인이하일경우 0으로 초기화 하였으므로 유저가 너무 적다는 메시지 출력
			printf("참여하는 유저수가 너무 적습니다.\n\n");
		else if(rcv_data.value == input && rcv_data.pid != getpid())
			printf("중간값은 맞췄지만 너무 늦었습니다.... 좀더 빨리 전송하세요! [중간값 : %d]\n\n", input);
		else
			printf("이번턴 중간값찾기 패자입니다. 중간값은 ( %d ) 입니다 [보냈던값 : %d]\n\n", rcv_data.value, input);
	}
	return 0;
}

