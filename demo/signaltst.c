#include <signal.h>
#include <unistd.h>
#include <stdio.h>
//void sig_hander(int signum){
//	printf("catch the signal %d\n",signum);
//	return ;
//}

sig_atomic_t signaled = 0;

void sig_hander(int signum){
	signaled = !signaled;
}

int main(int argc, char **argv){
	pid_t pid;
	pid = getpid();
	printf("pid[%d]\n",getpid());
	//if(signal(SIGINT,sig_hander)==SIG_ERR){
	//	printf("catch err!\n");
	//	return -1;
	//}
	signal(SIGINT,sig_hander);
	while(1){
		printf("signaled is %d.\n",signaled);
	}
	return 0;
}
