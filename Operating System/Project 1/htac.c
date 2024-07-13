#include "types.h"
#include "user.h"

int line; // count that prints lines from back
char buf[512]; // temp buffer

void htac(int fd) {
    int line_count = 0, data_sz = 0;
    char *data = 0, *nxt;

    {
        int n;
        while ((n = read(fd, buf, 512)) > 0) { // read until EOF
            for (int i = 0; i < n; i++) line_count += buf[i] == '\n'; // counts '\n'
            nxt = malloc(data_sz + n); // there is no realloc, so allocate new space, and move data to new space from previous space.
            for (int i = 0; i < data_sz; i++) nxt[i] = data[i];
            if (data) free(data); // free previous space.

            for (int i = 0; i < n; i++) nxt[i + data_sz] = buf[i]; // push back data from buffer
            data = nxt;
            data_sz += n;
        }
    }

    for (int i = line_count; i >= 0 && i > line_count - line; i--) { // read from back
        int curr = 0;
        for (int j = 0; j < data_sz; j++) {
            if (data[j] == '\n') curr++; // if meet '\n', add increase current line count.
            else if (i == curr) write(1, data + j, 1); // write current character to stdout when current line equals "i".
        }
        write(1, "\n", 1); // print new line.
    }

    free(data);
}

int main(int argc, char **argv) {
    int fd, i;

    if (argc <= 1) {
        htac(0);
        exit();
    }

    for(i = 1; i + 1 < argc; i += 2){
        line = atoi(argv[i]); // char* to int.
        if((fd = open(argv[i + 1], 0)) < 0){ // open file
            printf(1, "htac: cannot open %s\n", argv[i + 1]);
            exit();
        }
        htac(fd); // call htac function with file descriptor
        close(fd); // close file
    }
    exit(); // exit program.
}
