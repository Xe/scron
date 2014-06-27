#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <syslog.h>

#define LEN 256

char config[LEN+1] = "/etc/crontab";

void arg(int argc, char *argv[]) {
	int i;
	pid_t pid;

	for (i = 1; i < argc; i++) {
		if (!strcmp("-d", argv[i])) {
			pid = fork();
			if (pid < 0) {
				fprintf(stderr, "error: failed to fork daemon\n");
				syslog(LOG_NOTICE, "error: failed to fork daemon");
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
				syslog(LOG_NOTICE, "error: -f needs parameter");
				exit(EXIT_FAILURE);
			}
			strncpy(config, argv[++i], LEN);
		} else {
			fprintf(stderr, "usage: %s [options]\n", argv[0]);
			fprintf(stderr, "-h        help\n");
			fprintf(stderr, "-d        daemon\n");
			fprintf(stderr, "-f <file> config file\n");
			exit(EXIT_FAILURE);
		}
	}
}

int parsecolumn(char *col, int y, int x) {
	int entry, entry2;
	char *endptr, *endptr2;
	time_t t;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	if (x >= 0 && x <= 4) {
		endptr = "";
		entry = strtol(col, &endptr, 0);
		if (!strcmp("*", col)) {
			return 0;
		} else if (*endptr == '-') {
			endptr++;
			endptr2 = "";
			entry2 = strtol(endptr, &endptr2, 0);
			if (*endptr2 != '\0' || entry2 < 0 || entry2 > 59) {
				fprintf(stderr, "error: %s line %d column %d\n", config, y+1, x+1);
				syslog(LOG_NOTICE, "error: %s line %d column %d", config, y+1, x+1);
				return 1;
			} else if ((x == 0 && entry <= tm->tm_min && entry2 >= tm->tm_min) ||
					(x == 1 && entry <= tm->tm_hour && entry2 >= tm->tm_hour) ||
					(x == 2 && entry <= tm->tm_mday && entry2 >= tm->tm_mday) ||
					(x == 3 && entry <= tm->tm_mon && entry2 >= tm->tm_mon) ||
					(x == 4 && entry <= tm->tm_wday && entry2 >= tm->tm_wday)) {
				return 0;
			}
		} else if (*endptr != '\0' || entry < 0 || entry > 59) {
			fprintf(stderr, "error: %s line %d column %d\n", config, y+1, x+1);
			syslog(LOG_NOTICE, "error: %s line %d column %d", config, y+1, x+1);
			return 1;
		} else if ((x == 0 && entry == tm->tm_min) ||
				(x == 1 && entry == tm->tm_hour) ||
				(x == 2 && entry == tm->tm_mday) ||
				(x == 3 && entry == tm->tm_mon) ||
				(x == 4 && entry == tm->tm_wday)) {
			return 0;
		}
	}

	return 1;
}

void runjob(char *cmd) {
	time_t t;
	pid_t pid;

	t = time(NULL);

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "error: failed to fork job: %s time: %s", cmd, ctime(&t));
		syslog(LOG_NOTICE, "error: failed to fork job: %s", cmd);
	} else if (pid == 0) {
		printf("run: %s pid: %d time: %s", cmd, (int) getpid(), ctime(&t));
		syslog(LOG_NOTICE, "run: %s pid: %d", cmd, (int) getpid());
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char *) NULL);
		fprintf(stderr, "error: job failed: %s time: %s\n", cmd, ctime(&t));
		syslog(LOG_NOTICE, "error: job failed: %s", cmd);
		exit(EXIT_FAILURE);
	}
}

void checkreturn(void) {
	int status;
	time_t t;
	pid_t pid;

	t = time(NULL);

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		printf("complete: pid %d, return: %d time: %s", (int) pid, status, ctime(&t));
		syslog(LOG_NOTICE, "complete: pid: %d return: %d", (int) pid, status);
	}
}

int main(int argc, char *argv[]) {
	int y, x;
	char line[LEN+1];
	char *col;
	time_t t;
	FILE *fp;

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);

	arg(argc, argv);

	while (1) {
		t = time(NULL);
		sleep(60 - t % 60);

		if ((fp = fopen(config, "r")) == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			syslog(LOG_NOTICE, "error: cant read %s", config);
			continue;
		}

		for (y = 0; fgets(line, LEN+1, fp); y++) {
			if (line[0] == '#' || line[0] == '\n')
				continue;

			strtok(line, "\n");

			for (x = 0, col = strtok(line,"\t"); col; x++, col = strtok(NULL, "\t")) {
				if (!parsecolumn(col, y, x))
					continue;
				else if (x == 5)
					runjob(col);

				break;
			}
		}

		fclose(fp);
		checkreturn();
	}

	closelog();
	return 0;
}
