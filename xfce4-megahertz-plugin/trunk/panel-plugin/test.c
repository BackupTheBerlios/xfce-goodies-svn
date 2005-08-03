#include <stdio.h>

main() {
int mhz=0;
FILE *f;

f=fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
if (f) {
    fscanf(f, "%d", &mhz);
    if (mhz/1000 > 1000)
        printf("MHZ: %d\n", mhz/1000); // wyswietla MHz u gory
    else
	printf("MHZ: %.2f\n", mhz/1e6); // wyswietla GHz u gory
    fclose(f);
    }
    
}
