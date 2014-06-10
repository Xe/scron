#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>

#define MAXLEN 100
#define SLEEP 60

static const char config[] = "/etc/dcron.conf";

FILE *fp;

void inthandler(void) {
	syslog(LOG_NOTICE, "quit");
	closelog();
	fclose(fp);
	exit(0);
}

int main(int argc, char *argv[]) {
	char line[MAXLEN+1];
	char *col;
	int min, hour, mday, mon, wday;
	char cmd[MAXLEN+1];
	int i;
	time_t t;
	struct tm *tm;

	if (argc > 1 && !strcmp("-h", argv[1])) {
		fprintf(stderr, "usage: %s [-h = help] [-d = daemon]\n", argv[0]);
		return 1;
	} else if (argc > 1 && !strcmp("-d", argv[1])) {
		if (!daemon(1, 0))
			return 1;
	}

	signal(SIGINT, inthandler);
	
	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);
	syslog(LOG_NOTICE, "start uid:%d", getuid());

	while (1) {
		t = time(NULL);
		tm = localtime(&t);

		fp = fopen(config, "r");
		if (fp == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			sleep(SLEEP);
			continue;
		}

		min = hour = mday = mon = wday = 0;

		while (fgets(line, MAXLEN+1, fp) != NULL) {
			if (line[1] != '\0' && line[0] != '\043') {
				col = strtok(line,"\t");
				i = 0;
				while (col != NULL) {
					switch (i) {
						case 0:
							if (isalnum(col[0]))
								min = strtol(col, NULL, 0);
							else
								min = -1;
							break;
						case 1:
							if (isalnum(col[0]))
								hour = strtol(col, NULL, 0);
							else
								hour = -1;
							break;
						case 2:
							if (isalnum(col[0]))
								mday = strtol(col, NULL, 0);
							else
								mday = -1;
							break;
						case 3:
							if (isalnum(col[0]))
								mon = strtol(col, NULL, 0);
							else
								mon = -1;
							break;
						case 4:
							if (isalnum(col[0]))
								wday = strtol(col, NULL, 0);
							else
								wday = -1;
							break;
						case 5:
							strncpy(cmd, col, MAXLEN+1);
							break;
					}
					col = strtok(NULL, "\t");
					i++;
				}

				if (min == -1 || min == tm->tm_min) {
					if (hour == -1 || hour == tm->tm_hour) {
						if (mday == -1 || mday == tm->tm_mday) {
							if (mon == -1 || mon == tm->tm_mon) {
								if (wday == -1 || wday == tm->tm_wday) {
									printf("dcron %.2d:%.2d %.2d.%.2d.%.4d\n",
											tm->tm_hour, tm->tm_min,
											tm->tm_mday, tm->tm_mon, tm->tm_year+1900);

									printf("run: %s", cmd);
									syslog(LOG_NOTICE, "run: %s", cmd);
									if (system(cmd))
										puts("ok");
								}
							}
						}
					}
				}
			}
		}
		sleep(SLEEP);
	}
	inthandler();
	return 0;
}
