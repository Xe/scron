#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>

#define LEN 100

int main(int argc, char *argv[]) {
	char *col, line[LEN+1], config[LEN+1] = "/etc/dcron.conf";
	int i, j;
	time_t t;
	struct tm *tm;
	FILE *fp;

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);

	for (i = 1; i < argc; i++) {
		if (!strcmp("-d", argv[i])) {
			if (daemon(1, 0) != 0) {
				fprintf(stderr, "error: failed to daemonize\n");
				syslog(LOG_NOTICE, "error: failed to daemonize");
				return 1;
			}
		} else if (!strcmp("-f", argv[i])) {
			if (argv[i+1] == NULL || argv[i+1][0] == '-') {
				fprintf(stderr, "error: -f needs parameter\n");
				syslog(LOG_NOTICE, "error: -f needs parameter");
				return 1;
			}
			strncpy(config, argv[++i], LEN);
			printf("config: %s\n", config);
			syslog(LOG_NOTICE, "config: %s", config);
		} else {
			fprintf(stderr, "usage: %s [options]\n", argv[0]);
			fprintf(stderr, "-h        help\n");
			fprintf(stderr, "-d        daemon\n");
			fprintf(stderr, "-f <file> config file\n");
			return 1;
		}
	}

	for (;; sleep(60)) {
		t = time(NULL);
		tm = localtime(&t);

		if ((fp = fopen(config, "r")) == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			syslog(LOG_NOTICE, "error: cant read %s", config);
			continue;
		}

		for (i = 0; fgets(line, LEN+1, fp) != NULL; i++) {
			if (line[1] == '\0' || line[0] == '\043')
				continue;

			for (j = 0, col = strtok(line,"\t"); col != NULL; j++, col = strtok(NULL, "\t")) {
				if ((j == 0 && (ispunct(col[0]) || strtol(col, NULL, 0) == tm->tm_min)) ||
						(j == 1 && (ispunct(col[0]) || strtol(col, NULL, 0) == tm->tm_hour)) ||
						(j == 2 && (ispunct(col[0]) || strtol(col, NULL, 0) == tm->tm_mday)) ||
						(j == 3 && (ispunct(col[0]) || strtol(col, NULL, 0) == tm->tm_mon)) ||
						(j == 4 && (ispunct(col[0]) || strtol(col, NULL, 0) == tm->tm_wday))) {
					continue;
				} else if (j == 5) {
					printf("run: %s", col);
					syslog(LOG_NOTICE, "run: %s", col);
					if (system(col) != 0) {
						fprintf(stderr, "error: job failed\n");
						syslog(LOG_NOTICE, "error: job failed");
					}
				} else if (!isdigit(col[0])) {
					fprintf(stderr, "error: %s line %d column %d\n", config, i+1, j+1);
					syslog(LOG_NOTICE, "error: %s line %d column %d", config, i+1, j+1);
				}
				break;
			}
		}
		fclose(fp);
	}
	closelog();
	return 0;
}
