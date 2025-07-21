#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

float getCpuUsage()
{

    FILE *filePath = fopen("/proc/stat", "r");
    if (!filePath) return -1;

    char buffer[1024];
    fgets(buffer, sizeof(buffer), filePath);
    fclose(filePath);

    unsigned long user, nice, system, idle;
    sscanf(buffer, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);

    static unsigned long prev_total = 0, prev_idle = 0;
    unsigned long total = user + nice + system + idle;
    unsigned long total_diff = total - prev_total;
    unsigned long idle_diff = idle - prev_idle;

    prev_total = total;
    prev_idle = idle;

    if (total_diff == 0) return 0;
    return 100.0 * (total_diff - idle_diff) / total_diff;

}

void getRamUsage(long* usedMb, long* totalMb, float* percent)
{

    FILE* filePath = fopen("/proc/meminfo", "r");
    if (!filePath) return;

    char label[64];
    long memTotal = 0;
    long memFree = 0;
    long buffers = 0;
    long cached = 0;

    while (fscanf(filePath, "%s %ld kB\n", label, &memTotal) == 2) 
    {

        if (strcmp(label, "MemTotal:") == 0) *totalMb = memTotal / 1024;
        else if (strcmp(label, "MemFree:") == 0) memFree = memTotal;
        else if (strcmp(label, "Buffers:") == 0) buffers = memTotal;
        else if (strcmp(label, "Cached:") == 0) cached = memTotal;

        if (*totalMb && memFree && buffers && cached) break;
            
    }

    fclose(filePath);

    long available = memFree + buffers + cached;
    *usedMb = (*totalMb * 1024 - available) / 1024;
    *percent = 100.0 * (*usedMb) / (*totalMb);

}

void showSystemMonitor()
{

    float cpu = getCpuUsage();
    long used = 0;
    long total = 0;
    float ramPercent = 0;

    getRamUsage(&used, &total, &ramPercent);

    printf("=== System Monitor ===\n");
    printf("CPU Usage: %.2f%%\n", cpu);
    printf("RAM Usage: %ldMB / %ldMB (%.2f%%)\n", used, total, ramPercent);

    printf("Disk Usage:\n");
    fflush(stdout);
    system("df -h --output=source,pcent,size,used,avail | grep '^/'");

}
