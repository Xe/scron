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

static char config[PATH_MAX] = "/etc/crontab";

static void
arg(int argc, char *argv[])
{
	int i;
	pid_t pid;

	for (i = 1; i < argc; i++) {
		if (!strcmp("-d", argv[i])) {
			pid = fork();
			if (pid < 0) {
				fprintf(stderr, "error: failed to fork daemon\n");
				syslog(LOG_WARNING, "error: failed to fork daemon");
			} else if (pid == 0) {
				setsid();
				fclose(stdin);
				fclose(stdout);
				fclose(stderr);
			} else {
				exit(EXIT_SUCCESS);
			}
		} else if (!strcmp("-f", argv[i])) {
			if (argv[i+1] == NULL || argv[i+1][0] == '-') {
				fprintf(stderr, "error: -f needs parameter\n");
				syslog(LOG_WARNING, "error: -f needs parameter");
				exit(EXIT_FAILURE);
			}
			strncpy(config, argv[++i], PATH_MAX-1);
		} else {
			fprintf(stderr, "usage: %s [options]\n", argv[0]);
			fprintf(stderr, "-h        help\n");
			fprintf(stderr, "-d        daemon\n");
			fprintf(stderr, "-f <file> config file\n");
			exit(EXIT_FAILURE);
		}
	}
}

static int
parsecolumn(char *col, int y, int x)
{
	int value, value2;
	char *endptr, *endptr2;
	time_t t;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	if (x >= 0 && x <= 4) {
		endptr = "";
		endptr2 = "";
		value = strtol(col, &endptr, 0);
		value2 = -1;
		if (*endptr == '-') {
			endptr++;
			value2 = strtol(endptr, &endptr2, 0);
			endptr = "";
		}

		if (!strcmp("*", col)) {
			return 0;
		} else if (*endptr != '\0' || *endptr2 != '\0') {
			fprintf(stderr, "error: %s line %d column %d\n", config, y+1, x+1);
			syslog(LOG_WARNING, "error: %s line %d column %d", config, y+1, x+1);
		} else if (value2 == -1) {
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
	}

	return 1;
}

static void
runjob(char *cmd)
{
	time_t t;
	pid_t pid;

	t = time(NULL);

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "error: failed to fork job: %s time: %s", cmd, ctime(&t));
		syslog(LOG_WARNING, "error: failed to fork job: %s", cmd);
	} else if (pid == 0) {
		printf("run: %s pid: %d time: %s", cmd, getpid(), ctime(&t));
		fflush(stdout);
		syslog(LOG_INFO, "run: %s pid: %d", cmd, getpid());
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
		fprintf(stderr, "error: job failed: %s time: %s\n", cmd, ctime(&t));
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
		printf("complete: pid %d, return: %d time: %s", pid, status, ctime(&t));
		fflush(stdout);
		syslog(LOG_INFO, "complete: pid: %d return: %d", pid, status);
	}
}

int
main(int argc, char *argv[])
{
	int y, x;
	char line[PATH_MAX];
	char *col;
	time_t t;
	FILE *fp;

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_DAEMON);

	arg(argc, argv);

	if ((fp = fopen(config, "r")) == NULL) {
		fprintf(stderr, "error: cant read %s\n", config);
		syslog(LOG_WARNING, "error: cant read %s", config);
		closelog();
		return EXIT_FAILURE;
	}

	while (1) {
		t = time(NULL);
		sleep(60 - t % 60);

		for (y = 0; fgets(line, sizeof(line), fp); y++) {
			if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
				continue;
			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';

			for (x = 0, col = strtok(line, "\t"); col; x++, col = strtok(NULL, "\t")) {
				if (!parsecolumn(col, y, x))
					continue;
				if (x == 5)
					runjob(col);
				break;
			}
		}

		rewind(fp);
		waitjob();
	}

	fclose(fp);
	closelog();
	return EXIT_SUCCESS;
}
