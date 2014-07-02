/* See LICENSE file for copyright and license details. */
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "arg.h"

#define VERSION "0.1"

char *argv0;
static char *config = "/etc/crontab";
static int dflag;

static int
validfield(int x, int value)
{
	if (x == 0 && value >= 0 && value <= 59)
		return 1;
	if (x == 1 && value >= 0 && value <= 23)
		return 1;
	if (x == 2 && value >= 1 && value <= 31)
		return 1;
	if (x == 3 && value >= 1 && value <= 12)
		return 1;
	if (x == 4 && value >= 0 && value <= 6)
		return 1;
	return 0;
}

static int
parsecolumn(char *col, int y, int x)
{
	int value, value2;
	char *endptr, *endptr2;
	time_t t;
	struct tm *tm;

	if (x < 0 || x > 4)
		return -1;

	if (strcmp("*", col) == 0)
		return 0;

	/* parse element */
	endptr = "";
	endptr2 = "";
	value = strtol(col, &endptr, 0);
	value2 = -1;
	if (*endptr == '-') {
		endptr++;
		value2 = strtol(endptr, &endptr2, 0);
		endptr = "";
	}

	if (*endptr != '\0' || *endptr2 != '\0') {
		if (dflag == 1)
			syslog(LOG_WARNING, "error: %s line %d column %d",
			       config, y + 1, x + 1);
		fprintf(stderr, "error: %s line %d column %d\n",
			config, y + 1, x + 1);
		return -1;
	}

	/* check if element is valid */
	if ((value != -1 && validfield(x, value) == 0) ||
	    (value2 != -1 && validfield(x, value2) == 0)) {
		if (dflag == 1)
			syslog(LOG_WARNING, "error: %s line %d column %d",
			       config, y + 1, x + 1);
		fprintf(stderr, "error: %s line %d column %d\n",
			config, y + 1, x + 1);
	}

	t = time(NULL);
	tm = localtime(&t);

	/* check if we have a match */
	if (value2 == -1) {
		if ((x == 0 && value == tm->tm_min) ||
		    (x == 1 && value == tm->tm_hour) ||
		    (x == 2 && value == tm->tm_mday) ||
		    (x == 3 && value == tm->tm_mon) ||
		    (x == 4 && value == tm->tm_wday))
			return 0;
	} else {
		if ((x == 0 && value <= tm->tm_min && value2 >= tm->tm_min) ||
		    (x == 1 && value <= tm->tm_hour && value2 >= tm->tm_hour) ||
		    (x == 2 && value <= tm->tm_mday && value2 >= tm->tm_mday) ||
		    (x == 3 && value <= tm->tm_mon && value2 >= tm->tm_mon) ||
		    (x == 4 && value <= tm->tm_wday && value2 >= tm->tm_wday))
			return 0;
	}

	return -1;
}

static void
runjob(char *cmd)
{
	time_t t;
	pid_t pid;

	t = time(NULL);

	pid = fork();
	if (pid < 0) {
		if (dflag == 1)
			syslog(LOG_WARNING, "error: failed to fork job: %s", cmd);
		fprintf(stderr, "error: failed to fork job: %s time: %s", cmd, ctime(&t));
	} else if (pid == 0) {
		printf("run: %s pid: %d time: %s", cmd, getpid(), ctime(&t));
		fflush(stdout);
		if (dflag == 1)
			syslog(LOG_INFO, "run: %s pid: %d", cmd, getpid());
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
		fprintf(stderr, "error: job failed: %s time: %s\n", cmd, ctime(&t));
		if (dflag == 1)
			syslog(LOG_WARNING, "error: job failed: %s", cmd);
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
		if (WIFEXITED(status) == 1) {
			printf("complete: pid %d, return: %d time: %s",
			       pid, WEXITSTATUS(status), ctime(&t));
			fflush(stdout);
			if (dflag == 1)
				syslog(LOG_INFO, "complete: pid: %d return: %d",
				       pid, WEXITSTATUS(status));
		}
	}
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
	char line[PATH_MAX];
	char *col;
	int y, x;
	time_t t;
	FILE *fp;

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

	if (dflag == 1) {
		openlog(argv[0], LOG_CONS | LOG_PID, LOG_DAEMON);
		daemon(0, 0);
	}

	while (1) {
		t = time(NULL);
		sleep(60 - t % 60);

		if ((fp = fopen(config, "r")) == NULL) {
			if (dflag == 1)
				syslog(LOG_WARNING, "error: cant read %s", config);
			fprintf(stderr, "error: cant read %s\n", config);
			continue;
		}

		for (y = 0; fgets(line, sizeof(line), fp); y++) {
			if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
				continue;
			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';

			for (x = 0, col = strtok(line, "\t"); col; x++, col = strtok(NULL, "\t")) {
				if (parsecolumn(col, y, x) == 0)
					continue;
				if (x == 5)
					runjob(col);
				break;
			}
		}

		fclose(fp);
		waitjob();
	}

	if (dflag == 1)
		closelog();

	return EXIT_SUCCESS;
}
