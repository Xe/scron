/* See LICENSE file for copyright and license details. */
#include <sys/types.h>
#include <sys/wait.h>

#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "arg.h"
#include "queue.h"

#define VERSION "0.1"

#define LEN(x) (sizeof (x) / sizeof *(x))

struct field {
	/* [low, high] */
	long low;
	long high;
	/* for every `div' units */
	long div;
};

struct ctabentry {
	struct field min;
	struct field hour;
	struct field mday;
	struct field mon;
	struct field wday;
	char *cmd;
	TAILQ_ENTRY(ctabentry) entry;
};

char *argv0;
static sig_atomic_t reload;
static TAILQ_HEAD(ctabhead, ctabentry) ctabhead;
static char *config = "/etc/crontab";
static int dflag;

static void
loginfo(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (dflag == 1)
		vsyslog(LOG_INFO, fmt, ap);
	vfprintf(stdout, fmt, ap);
	fflush(stdout);
	va_end(ap);
}

static void
logerr(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (dflag == 1)
		vsyslog(LOG_ERR, fmt, ap);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void *
emalloc(size_t size)
{
	void *p;
	p = malloc(size);
	if (!p) {
		logerr("error: out of memory\n");
		exit(EXIT_FAILURE);
	}
	return p;
}

static char *
estrdup(const char *s)
{
	char *p;

	p = strdup(s);
	if (!p) {
		logerr("error: out of memory\n");
		exit(EXIT_FAILURE);
	}
	return p;
}

static void
runjob(char *cmd)
{
	time_t t;
	pid_t pid;

	t = time(NULL);

	pid = fork();
	if (pid < 0) {
		logerr("error: failed to fork job: %s at %s",
		       cmd, ctime(&t));
	} else if (pid == 0) {
		loginfo("run: %s pid: %d at %s",
			cmd, getpid(), ctime(&t));
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
		logerr("error: failed to execute job: %s at %s",
		       cmd, ctime(&t));
		_exit(EXIT_FAILURE);
	}
}

static void
waitjob(void)
{
	int status;
	time_t t;
	pid_t pid;

	t = time(NULL);

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		if (WIFSIGNALED(status) == 1) {
			loginfo("complete: pid: %d terminated by signal: %d time: %s",
				pid, WTERMSIG(status), ctime(&t));
			continue;
		}
		if (WIFEXITED(status) == 1) {
			loginfo("complete: pid: %d, return: %d time: %s",
				pid, WEXITSTATUS(status), ctime(&t));
			continue;
		}
	}
}

static int
matchentry(struct ctabentry *cte, struct tm *tm)
{
	struct {
		struct field *f;
		int tm;
	} matchtbl[] = {
		{ .f = &cte->min,  .tm = tm->tm_min  },
		{ .f = &cte->hour, .tm = tm->tm_hour },
		{ .f = &cte->mday, .tm = tm->tm_mday },
		{ .f = &cte->mon,  .tm = tm->tm_mon  },
		{ .f = &cte->wday, .tm = tm->tm_wday },
	};
	size_t i;

	for (i = 0; i < LEN(matchtbl); i++) {
		/* this is the match-any case, '*' */
		if (matchtbl[i].f->low == -1 && matchtbl[i].f->high == -1)
			continue;
		if (matchtbl[i].f->high == -1) {
			if (matchtbl[i].f->low == matchtbl[i].tm)
				continue;
			else if (matchtbl[i].tm % matchtbl[i].f->div == 0)
				continue;
		} else {
			if (matchtbl[i].f->low <= matchtbl[i].tm &&
			    matchtbl[i].f->high >= matchtbl[i].tm)
				continue;
		}
		break;
	}
	if (i != LEN(matchtbl))
		return 0;
	return 1;
}

static int
parsefield(const char *field, long low, long high, struct field *f)
{
	long min, max, div;
	char *e1, *e2;

	if (strcmp(field, "*") == 0) {
		f->low = -1;
		f->high = -1;
		return 0;
	}

	div = -1;
	max = -1;
	min = strtol(field, &e1, 10);

	switch (e1[0]) {
	case '-':
		e1++;
		errno = 0;
		max = strtol(e1, &e2, 10);
		if (e2[0] != '\0' || errno != 0)
			return -1;
		break;
	case '*':
		e1++;
		if (e1[0] != '/')
			return -1;
		e1++;
		errno = 0;
		div = strtol(e1, &e2, 10);
		if (e2[0] != '\0' || errno != 0)
			return -1;
		break;
	case '\0':
		break;
	default:
		return -1;
	}

	if (min < low || min > high)
		return -1;
	if (max != -1)
		if (max < low || max > high)
			return -1;

	f->low = min;
	f->high = max;
	f->div = div;
	return 0;
}

static void
unloadentries(void)
{
	struct ctabentry *cte, *tmp;

	for (cte = TAILQ_FIRST(&ctabhead); cte; cte = tmp) {
		tmp = TAILQ_NEXT(cte, entry);
		TAILQ_REMOVE(&ctabhead, cte, entry);
		free(cte->cmd);
		free(cte);
	}
}

static int
loadentries(void)
{
	struct ctabentry *cte;
	FILE *fp;
	char *line = NULL, *p, *col;
	int r = 0, y;
	size_t size = 0;
	ssize_t len;

	if ((fp = fopen(config, "r")) == NULL) {
		logerr("error: can't open %s\n", config);
		return -1;
	}

	for (y = 0; (len = getline(&line, &size, fp)) != -1; y++) {
		p = line;
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;
		if(line[len - 1] == '\n')
			line[--len] = '\0';

		cte = emalloc(sizeof(*cte));

		col = strsep(&p, "\t");
		if (!col || parsefield(col, 0, 59, &cte->min) < 0) {
			logerr("error: failed to parse `min' field on line %d\n",
			       y + 1);
			free(cte);
			r = -1;
			break;
		}

		col = strsep(&p, "\t");
		if (!col || parsefield(col, 0, 23, &cte->hour) < 0) {
			logerr("error: failed to parse `hour' field on line %d\n",
			       y + 1);
			free(cte);
			r = -1;
			break;
		}

		col = strsep(&p, "\t");
		if (!col || parsefield(col, 1, 31, &cte->mday) < 0) {
			logerr("error: failed to parse `mday' field on line %d\n",
			       y + 1);
			free(cte);
			r = -1;
			break;
		}

		col = strsep(&p, "\t");
		if (!col || parsefield(col, 1, 12, &cte->mon) < 0) {
			logerr("error: failed to parse `mon' field on line %d\n",
			       y + 1);
			free(cte);
			r = -1;
			break;
		}

		col = strsep(&p, "\t");
		if (!col || parsefield(col, 0, 6, &cte->wday) < 0) {
			logerr("error: failed to parse `wday' field on line %d\n",
			       y + 1);
			free(cte);
			r = -1;
			break;
		}

		col = strsep(&p, "\n");
		if (!col) {
			logerr("error: missing `cmd' field on line %d\n",
			       y + 1);
			free(cte);
			r = -1;
			break;
		}
		cte->cmd = estrdup(col);

		TAILQ_INSERT_TAIL(&ctabhead, cte, entry);
	}

	if (r < 0)
		unloadentries();

	fclose(fp);
	free(line);

	return r;
}

static void
reloadentries(void)
{
	unloadentries();
	loadentries();
}

static void
sighup(int sig)
{
	if (sig == SIGHUP)
		reload = 1;
}

static void
usage(void)
{
	fprintf(stderr, VERSION " (c) 2014\n");
	fprintf(stderr, "usage: %s [-d] [-f file] [options]\n", argv0);
	fprintf(stderr, "  -d	daemonize\n");
	fprintf(stderr, "  -f	config file\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct ctabentry *cte;
	time_t t;
	struct tm *tm;

	ARGBEGIN {
	case 'd':
		dflag = 1;
		break;
	case 'f':
		config = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND;

	if (argc > 0)
		usage();

	TAILQ_INIT(&ctabhead);

	if (dflag == 1) {
		openlog(argv[0], LOG_CONS | LOG_PID, LOG_DAEMON);
		daemon(0, 0);
	}

	signal(SIGHUP, sighup);

	loadentries();

	while (1) {
		t = time(NULL);
		sleep(60 - t % 60);

		if (reload == 1) {
			reloadentries();
			reload = 0;
			continue;
		}

		TAILQ_FOREACH(cte, &ctabhead, entry) {
			t = time(NULL);
			tm = localtime(&t);
			if (matchentry(cte, tm) == 1)
				runjob(cte->cmd);
		}

		waitjob();
	}

	if (dflag == 1)
		closelog();

	return EXIT_SUCCESS;
}
