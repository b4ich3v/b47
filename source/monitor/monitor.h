#ifndef MONITOR_H
#define MONITOR_H

float getCpuUsage(void);
void getRamUsage(long* usedMb, long* totalMb, float* percent);
void showSystemMonitor(void);

#endif
