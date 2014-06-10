#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAXLEN 100

int main(void) {
	FILE *fp;
	char line[MAXLEN+1];
	char *col;
	char cmd[MAXLEN+1];
	int i, min, hour;
	time_t rawtime;
	struct tm *tmtime;

	while (1) {
		rawtime = time(NULL);
		tmtime = localtime(&rawtime);
		
		printf("dcron %.2d:%.2d %.2d.%.2d.%.4d\n",
				tmtime->tm_hour, tmtime->tm_min,
				tmtime->tm_mday, tmtime->tm_mon, tmtime->tm_year+1900);

		fp = fopen("crontab", "r");
		if (fp == NULL) {
			fprintf(stderr, "cant read crontab\n");
			exit(1);
		}

		while (fgets(line, MAXLEN+1, fp) != NULL) {
			if (line[1] == '\0') {
				// empty line
			} else if (line[0] == '\043') {
				// comment
			} else {
				// split line
				i = 0;
				col = strtok (line,"\t");
				while (col != NULL) {
					// parse tasks
					switch (i) {
						case 0:
							min = strtol(col, NULL, 0);
							break;
						case 1:
							hour = strtol(col, NULL, 0);
							break;
						case 5:
							strncpy(cmd, col, MAXLEN+1);
							break;
					}

					col = strtok (NULL, "\t");
					i++;
				}

				if (hour == tmtime->tm_hour && min == tmtime->tm_min) {
					printf("run: %s", cmd);
					// system(cmd);
				}
			}
		}

		sleep(60);
	}

	fclose(fp);
	return 0;
}
