#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <syslog.h>

#define MAXLEN 100
#define SLEEP 60

// config file location
char config[MAXLEN+1] = "/etc/dcron.conf";

int main(int argc, char *argv[]) {
	FILE *fp;
	char *argv0;
	char line[MAXLEN+1];
	char cmd[MAXLEN+1];
	char *col;
	int min, hour, mday, mon, wday;
	int i;
	time_t t;
	struct tm *tm;

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);
	syslog(LOG_NOTICE, "start uid:%d", getuid());

	// parse commandline arguments
	for (argv0 = *argv; argc > 0; argc--, argv++) {
		if (!strcmp("-h", argv[0])) {
			fprintf(stderr, "usage: %s [options]\n\n", argv0);

			fprintf(stderr, "-h         help\n");
			fprintf(stderr, "-d         daemon\n");
			fprintf(stderr, "-f <file>  config file\n");
			return 1;
		} else if (!strcmp("-d", argv[0])) {
			if (daemon(1, 0) != 0) {
				fprintf(stderr, "error: failed to daemonize\n");
				syslog(LOG_NOTICE, "error: failed to daemonize");
				return 1;
			}
		} else if (!strcmp("-f", argv[0])) {
			if (argv[1][0] == '-') {
				fprintf(stderr, "error: -f needs parameter\n");
				syslog(LOG_NOTICE, "error: -f needs parameter");
				break;
			}
			strncpy(config, argv[1], MAXLEN);
			printf("config: %s\n", config);
			syslog(LOG_NOTICE, "config: %s", config);
			argc--, argv++;
		}
	}

	// main loop
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

		// get next line from config
		while (fgets(line, MAXLEN, fp) != NULL) {
			// skip comments and empty lines
			if (line[1] != '\0' && line[0] != '\043') {
				// split line to columns
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

				// skip task if any column is unset
				if (min == 0 || hour == 0 || mday == 0 || mon == 0 ||
						wday == 0 || cmd[0] == '\0' || cmd[1] == '\0') {
					fprintf(stderr, "error: %s line %d\n", config, i);
					syslog(LOG_NOTICE, "error: %s line %d", config, i);
					sleep(SLEEP);
					continue;
				}

				// run task if date matches
				if ((min == -1 || min == tm->tm_min) &&
						(hour == -1 || hour == tm->tm_hour) &&
						(mday == -1 || mday == tm->tm_mday) &&
						(mon == -1 || mon == tm->tm_mon) &&
						(wday == -1 || wday == tm->tm_wday)) {
					printf("run: %s", cmd);
					syslog(LOG_NOTICE, "run: %s", cmd);
					if (system(cmd) != 0) {
						fprintf(stderr, "error: job failed\n");
						syslog(LOG_NOTICE, "error: job failed");
					}
				}
			}
		}
		fclose(fp);
		sleep(SLEEP);
	}
	closelog();
	return 0;
}
