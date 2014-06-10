#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <syslog.h>

#define MAXLEN 100
#define SLEEP 60

static const char config[] = "/etc/dcron.conf";

int main(int argc, char *argv[]) {
	FILE *fp;
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
		daemon(1, 0);
	}

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);
	syslog(LOG_NOTICE, "start uid:%d", getuid());

	while (1) {
		t = time(NULL);
		tm = localtime(&t);
		min = hour = mday = mon = wday = 0;

		fp = fopen(config, "r");
		if (fp == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			syslog(LOG_NOTICE, "error: cant read %s", config);
			sleep(SLEEP);
			continue;
		}

		while (fgets(line, MAXLEN, fp) != NULL) {
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
							strncpy(cmd, col, MAXLEN);
							break;
					}
					col = strtok(NULL, "\t");
					i++;
				}

				if ((min == -1 || min == tm->tm_min) &&
						(hour == -1 || hour == tm->tm_hour) &&
						(mday == -1 || mday == tm->tm_mday) &&
						(mon == -1 || mon == tm->tm_mon) &&
						(wday == -1 || wday == tm->tm_wday)) {
					printf("run: %s", cmd);
					syslog(LOG_NOTICE, "run: %s", cmd);
					system(cmd);
				}
			}
		}
		fclose(fp);
		sleep(SLEEP);
	}
	closelog();
	return 0;
}
