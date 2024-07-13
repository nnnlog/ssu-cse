#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "ssu_employee.h"

int main(int argc, char *argv[]) {
	struct ssu_employee record;
	int fd;

	if ((fd = open(argv[1], O_WRONLY | O_EXCL | O_CREAT, 0666)) < 0) {
		fprintf(stderr, "open error\n");
		exit(1);
	}

//	memset(record, 0, sizeof(record));

	while (1) {
		printf("Enter employee name <SPACE> salary: ");
		scanf("%s", record.name);

		if (!strcmp(record.name, ".")) break;
		scanf("%d", &record.salary);
		write(fd, &record, sizeof (record));
	}

	close(fd);

	exit(0);
}
