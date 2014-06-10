#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>

#define MAXLEN 100

static const char config[]  = "cron.conf";

int main(void) {
	FILE *fp;
	char line[MAXLEN+1];
	char cmd[MAXLEN+1];
	char *col;
	int i, min, hour, mday, mon, wday;
	time_t rawtime;
	struct tm *tmtime;

	while (1) {
		rawtime = time(NULL);
		tmtime = localtime(&rawtime);
		
		printf("dcron %.2d:%.2d %.2d.%.2d.%.4d\n",
				tmtime->tm_hour, tmtime->tm_min,
				tmtime->tm_mday, tmtime->tm_mon, tmtime->tm_year+1900);

		fp = fopen(config, "r");
		if (fp == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			sleep(60);
			continue;
		}

		while (fgets(line, MAXLEN+1, fp) != NULL) {
			if (line[1] == '\0') {
				// empty line
			} else if (line[0] == '\043') {
				// comment
			} else {
				// split line
				i = 0;
				col = strtok(line,"\t");
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
					col = strtok (NULL, "\t");
					i++;
				}

				// run task
				if (min == -1 || min == tmtime->tm_min) {
					if (hour == -1 || hour == tmtime->tm_hour) {
						if (mday == -1 || mday == tmtime->tm_mday) {
							if (mon == -1 || mon == tmtime->tm_mon) {
								if (wday == -1 || wday == tmtime->tm_wday) {
									printf("run: %s", cmd);
									// system(cmd);
								}
							}
						}
					}
				}
			}
		}
		sleep(60);
	}
	fclose(fp);
	return 0;
}
