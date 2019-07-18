#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Ausgabe des Aufrufs des Programms
    for (int i = 0; i < argc; i++) {
        fprintf(stdout, "%s ", argv[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "VMEM_PAGESIZE=%d\n", VMEM_PAGESIZE);
    #ifdef STOP_BEFORE_END
    pause();
    #endif
    return 0;
}