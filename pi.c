#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
typedef struct timespec timespec;

int num_thread;
long total_loopcount;
double global_hit=0.0;
pthread_mutex_t mutex;
long hit[16];
long loop_cnt[16];
int idx = 0;
timespec last_time;

timespec diff(timespec start, timespec end)
{
	timespec temp;
	if ( (end.tv_nsec-start.tv_nsec)<0 ){
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

void ctr_c(int signum){
	timespec current_time;
	double current_pi=0.0;
	double current_hit=0;
	long current_loopcount=0;

	clock_gettime(CLOCK_MONOTONIC, &current_time);

	// 一秒內按第二次ctrl-c會中止程式
	timespec d = diff(last_time, current_time);
	last_time = current_time;
	if( d.tv_sec < 1 ){
		kill(getpid(), SIGKILL);
	}

	for(int i=0;i<idx;i++){
		current_hit += hit[i];
		current_loopcount += loop_cnt[i];
	}
	current_pi = (double)4*(current_hit/current_loopcount);
	printf("now, pi = %.8lf\n",current_pi);
}

void thread(void* rand_buffer) {
	double point_x,point_y;
	long i;
	int j;
	long local_loopcount = total_loopcount/num_thread;
	double rand_d;
	int thread_idx;

	pthread_mutex_lock(&mutex);
	thread_idx = idx++;
	pthread_mutex_unlock(&mutex);

	long* local_hit = &hit[thread_idx];

	long tmp_local_hit = *local_hit;
	for(j=0; j<100; j++){
		for(i=0;i<local_loopcount/100;i++) {	

			drand48_r(rand_buffer, &rand_d);
			point_x = rand_d;
			drand48_r(rand_buffer, &rand_d);
			point_y = rand_d;
			if( (point_x*point_x + point_y*point_y) < 1.0)
				tmp_local_hit+=1;
                //*local_hit+=1;

			//loop_cnt[thread_idx]++;
		}
		*local_hit = tmp_local_hit;
		loop_cnt[thread_idx]+=i;
	}   //實驗結果: 使用tmp_local_hit會快很多, 直接對*local_hit+=1卻很慢

	printf("hit = %ld\n", *local_hit);
	pthread_mutex_lock(&mutex);
	global_hit += *local_hit;
	pthread_mutex_unlock(&mutex);
}


void main(int argc,char*argv[]) {
	pthread_t id[16];
	struct drand48_data* rand_buffer[16];
	int i;
	double pi = 0.0;
	double rand_d;

	total_loopcount=atol(argv[1]);
	num_thread=atoi(argv[2]);
	assert(num_thread < 16);

	signal(SIGINT, ctr_c);

	pthread_mutex_init(&mutex,NULL);	
	for(i=0;i<num_thread;i++) {
		/*使用aligned_alloc分配記憶體，避免false sharing*/
		/*在這個例子中，「剛好」用malloc也會得到相同效果*/
		rand_buffer[i] = aligned_alloc(64, sizeof(struct drand48_data));
		srand48_r(rand(), rand_buffer[i]);
		drand48_r(rand_buffer[i], &rand_d);
		printf("@buffer = %p\n", rand_buffer[i]);
		printf("thread%d's seed = %f\n", i, rand_d);
		pthread_create(&id[i],NULL,(void *)thread,rand_buffer[i]);
	}

	for(i=0;i<num_thread;i++)
		pthread_join(id[i],NULL);

	pi = (double)4*(global_hit/total_loopcount);
	printf("pi = %.8lf\n",pi);
}
