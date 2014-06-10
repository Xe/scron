#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <syslog.h>

#define MAXLEN 100
#define SLEEP 60

static const char config[] = "cron.conf";

int main(int argc, char *argv[]) {
	FILE *fp;
	char line[MAXLEN+1];
	char *col;
	int min, hour, mday, mon, wday;
	char cmd[MAXLEN+1];
	int i;
	time_t rawtime;
	struct tm *tmtime;

	if (argc > 1 && !strcmp("-h", argv[1])) {
		fprintf(stderr, "usage: %s [-h = help] [-d = daemon]\n", argv[0]);
		return 1;
	} else if (argc > 1 && !strcmp("-d", argv[1])) {
		daemon(1, 0);
	}
	
	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);
	syslog(LOG_NOTICE, "started by user %d", getuid());

	while (1) {
		rawtime = time(NULL);
		tmtime = localtime(&rawtime);

		fp = fopen(config, "r");
		if (fp == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			sleep(SLEEP);
			continue;
		}

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

				if (min == -1 || min == tmtime->tm_min) {
					if (hour == -1 || hour == tmtime->tm_hour) {
						if (mday == -1 || mday == tmtime->tm_mday) {
							if (mon == -1 || mon == tmtime->tm_mon) {
								if (wday == -1 || wday == tmtime->tm_wday) {
									printf("dcron %.2d:%.2d %.2d.%.2d.%.4d\n",
											tmtime->tm_hour, tmtime->tm_min,
											tmtime->tm_mday, tmtime->tm_mon, tmtime->tm_year+1900);

									printf("run: %s", cmd);
									syslog(LOG_NOTICE, "run: %s", cmd);
									system(cmd);
								}
							}
						}
					}
				}
			}
		}
		sleep(SLEEP);
	}
	closelog();
	fclose(fp);
	return 0;
}
