/*
 * Author: Hongbo Wang(whbo158@qq.com)
 */

#include "rt_latency.h"

#ifdef XENOMAI_EVL_LIB
#include "evl/evl.h"
#endif


static uint32_t scycle_time = CYCLE_TIME;
static struct timespec swake_time;
static stat_val_t swake_stat;

static uint32_t stotal_sec = 0;
static uint32_t scur_sec = 0;

static pthread_t scycle_thread;
static int sprint_flag = 0;
static int sloop_flag = 1;
static int sexit_flag = 0;

static char sprep_cmd[] = {"uname -a"};
#ifdef XENOMAI_EVL_LIB
static char spost_cmd[] = {"evl ps -l"};
#else
static char spost_cmd[] = {""};
#endif

static int rt_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *req, struct timespec *rem)
{
#ifdef XENOMAI_EVL_LIB
	return evl_sleep_until(EVL_CLK_ID(clock_id), req);
#else
	return clock_nanosleep(clock_id, flags, req, rem);
#endif
}

static int rt_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
#ifdef XENOMAI_EVL_LIB
	return evl_read_clock(EVL_CLK_ID(clock_id), tp);
#else
	return clock_gettime(clock_id, tp);
#endif
}

static int rt_usleep(useconds_t usec)
{
#ifdef XENOMAI_EVL_LIB
	evl_usleep(usec);
#else
	usleep(usec);
#endif
}

static int print_test_result(void)
{
	stat_val_t *ps = &swake_stat;

	printf("sec,%d,cnt,%d,time,%d,%d,%d,%.2f,index,%d,%d\n", \
		scur_sec, ps->cnt, ps->min, ps->val, ps->max, ps->mean, ps->min_idx, ps->max_idx);

	return 0;
}

static void stat_max_min_time(stat_val_t *pval, struct timespec *pa, struct timespec *pb, uint32_t idx)
{
	int32_t diff = 0;

	diff = diff_time(pa, pb);
	pval->val = diff;

	if ((pval->cnt == 0) || (diff > pval->max)) {
		pval->max = diff;
		pval->max_idx = idx;
	}

	if ((pval->cnt == 0) || (diff < pval->min)) {
		pval->min = diff;
		pval->min_idx = idx;
	}

	pval->mean = (pval->mean * pval->cnt + pval->val) / (pval->cnt + 1.0);
	pval->cnt++;
}

static int run_system_cmd(char *pcmd)
{
	int ret = 0;

	if (strlen(pcmd) > 0) {
		ret = system(pcmd);
		rt_printf("\n");
	}

	return ret;
}

static void print_usage(char *pname)
{
	printf("\nRealtime Program Latency Test System\n\n");

	printf("Usage:\n %s -p / -c / -t / -h\n", pname);
	printf("\t-p: print test result every second\n");
	printf("\t-c: cycle_time, 125000ns as default\n");
	printf("\t-t: total test seconds, 0 as default\n");
	printf("\t-h: print usage\n");
	printf("\n");
}

static int parse_input_arg(int argc, char **argv)
{
	char *p = NULL;
	int ret = 0;
	int i = 0;

	for (i = 1; i < argc; i++) {
		p = argv[i];

		if (p[0] != '-')
			continue;

		switch (p[1]) {
		case 'p':
			sprint_flag = 1;
			break;
		case 'c':
			if ((i + 1) < argc)
				scycle_time = strtoul(argv[++i], NULL, 0);
			break;
		case 't':
			if ((i + 1) < argc)
				stotal_sec = strtoul(argv[++i], NULL, 0);
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			ret = 1;
			break;
		}
	}

	return ret;
}

static void *rt_latency_thread(void *p)
{
	struct timespec cycle_time;
	struct timespec wake_time;
	struct timespec curr_time;
	struct sched_param param;
	uint32_t cnt = 0;
	int ret = 0;

	prctl(PR_SET_NAME, "rt_latency_thread");

	memset(&param, 0, sizeof(param));

	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	ret = sched_setscheduler(getpid(), SCHED_FIFO, &param);
	if (ret == -1)
		printf("%s(): sched_setscheduler failed! ret:%d\n", __func__, ret);

	cycle_time.tv_nsec = scycle_time;
	cycle_time.tv_sec = 0;

#ifdef XENOMAI_EVL_LIB
	ret = evl_attach_self("/rt_latency_thread:%d", getpid());
	if (ret < 0)
		printf("%s: evl_attach_self() failed! ret:%d\n", __func__, ret);
	else
		rt_printf("%s: attched to XENOMAI EVL core!\n\n", __func__);
#endif

	rt_clock_gettime(CLOCK_TYPE, &wake_time);
	while (sloop_flag) {
		timespec_add(&wake_time, &cycle_time, &wake_time);

		ret = rt_clock_nanosleep(CLOCK_TYPE, TIMER_ABSTIME, &wake_time, NULL);
		if (ret) {
			rt_printf("failed to clock_nanosleep, error:%s\n", strerror(ret));
			break;
		}
		rt_clock_gettime(CLOCK_TYPE, &curr_time);

		stat_max_min_time(&swake_stat, &wake_time, &curr_time, cnt);
		cnt++;
	}

	while (sexit_flag == 1)
		rt_usleep(1000);

#ifdef XENOMAI_EVL_LIB
	ret = evl_detach_self();
	if (ret < 0)
		printf("%s: evl_detach_self() failed! ret:%d\n", __func__, ret);
	else
		printf("%s: detached from XENOMAI EVL core!\n", __func__);
#endif

	return NULL;
}

int start_rt_latency_thread(void)
{
	struct sched_param param;
	pthread_attr_t thattr;
	int ret = 0;

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
		printf("failed to lock memory: %s\n", strerror(errno));

	pthread_attr_init(&thattr);
	pthread_attr_setstacksize(&thattr, PTHREAD_STACK_MIN);

	ret = pthread_attr_setschedpolicy(&thattr, SCHED_FIFO);
	if (ret) {
		printf("pthread setschedpolicy failed\n");
		goto out_tag;
	}

	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	printf("%s(): sched_priority:%d cycle_time:%uns.\n", __func__, param.sched_priority, scycle_time);

	ret = pthread_attr_setschedparam(&thattr, &param);
	if (ret) {
		printf("pthread setschedparam failed\n");
		goto out_tag;
	}

	ret = pthread_attr_setinheritsched(&thattr, PTHREAD_EXPLICIT_SCHED);
	if (ret) {
		printf("pthread setinheritsched failed\n");
		goto out_tag;
	}

	ret = pthread_create(&scycle_thread, &thattr, &rt_latency_thread, NULL);
	if (ret) {
		printf("failed to create cyclic task, ret:%d-%s\n", ret, strerror(-ret));
		goto out_tag;
	}

out_tag:
	return ret;
}

static void signal_handler(int sig)
{
	sloop_flag = 0;
	sexit_flag = 1;
}

int main(int argc, char **argv)
{
	int ret = 0;

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	ret = parse_input_arg(argc, argv);
	if (ret)
		return ret;

	run_system_cmd(sprep_cmd);

	ret = start_rt_latency_thread();
	if (ret)
		return ret;

	while (sloop_flag) {
		if ((stotal_sec > 0) && (scur_sec >= stotal_sec)) {
			sloop_flag = 0;
			sexit_flag = 1;
			break;
		}

		if (sprint_flag && scur_sec)
			print_test_result();

		scur_sec++;
		sleep(1);
	}

	run_system_cmd(spost_cmd);
	sexit_flag = 2;

	printf("\nStatistic:\n");
	print_test_result();

	pthread_join(scycle_thread, NULL);

	return 0;
}
