#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <syslog.h>

#define MAXLEN 100

char config[MAXLEN+1] = "/etc/dcron.conf";

int main(int argc, char *argv[]) {
	FILE *fp;
	char *argv0, *col;
	char line[MAXLEN+1];
	char cmd[MAXLEN+1];
	int i, l, date[5];
	time_t t;
	struct tm *tm;

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);

	for (argv0 = argv[0]; argc > 0; argc--, argv++) {
		if (!strcmp("-h", argv[0])) {
			fprintf(stderr, "usage: %s [options]\n", argv0);
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
			if (argc < 2 || argv[1][0] == '-') {
				fprintf(stderr, "error: -f needs parameter\n");
				syslog(LOG_NOTICE, "error: -f needs parameter");
				return 1;
			}
			strncpy(config, argv[1], MAXLEN);
			printf("config: %s\n", config);
			syslog(LOG_NOTICE, "config: %s", config);
			argc--, argv++;
		}
	}

	printf("start uid:%d\n", getuid());
	syslog(LOG_NOTICE, "start uid:%d", getuid());

	while (1) {
		t = time(NULL);
		tm = localtime(&t);

		if ((fp = fopen(config, "r")) == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			syslog(LOG_NOTICE, "error: cant read %s", config);
			return 1;
		}

		for (l = 0; fgets(line, MAXLEN, fp) != NULL; l++) {
			if (line[1] == '\0' || line[0] == '\043')
				continue;

			for (col = strtok(line,"\t"), i = 0; col != NULL; col = strtok(NULL, "\t"), i++) {
				if (i == 5)
					strncpy(cmd, col, MAXLEN);
				else if (isdigit(col[0]))
					date[i] = strtol(col, NULL, 0);
				else if (ispunct(col[0]))
					date[i] = -1;
				else {
					date[i] = 0;
					fprintf(stderr, "error: %s line %d column %d\n", config, l+1, i+1);
					syslog(LOG_NOTICE, "error: %s line %d column %d", config, l+1, i+1);
				}
			}

			if ((date[0] == -1 || date[0] == tm->tm_min) &&
					(date[1] == -1 || date[1] == tm->tm_hour) &&
					(date[2] == -1 || date[2] == tm->tm_mday) &&
					(date[3] == -1 || date[3] == tm->tm_mon) &&
					(date[4] == -1 || date[4] == tm->tm_wday)) {
				printf("run: %s", cmd);
				syslog(LOG_NOTICE, "run: %s", cmd);
				if (system(cmd) != 0) {
					fprintf(stderr, "error: job failed\n");
					syslog(LOG_NOTICE, "error: job failed");
				}
			}
		}
		fclose(fp);
		sleep(60);
	}
	closelog();
	return 0;
}
