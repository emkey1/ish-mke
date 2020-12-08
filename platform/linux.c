#include <errno.h>
#include <sys/sysinfo.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/platform.h"
#include "debug.h"

static void read_proc_line(const char *file, const char *name, char *buf) {
    FILE *f = fopen(file, "r");
    if (f == NULL) ERRNO_DIE(file);
    do {
        fgets(buf, 1234, f);
        if (feof(f))
            die("could not find proc line %s", name);
    } while (!(strncmp(name, buf, strlen(name)) == 0 && buf[strlen(name)] == ' '));
    fclose(f);
}

struct cpu_usage get_total_cpu_usage() {
    struct cpu_usage usage = {};
    char buf[1234];
    read_proc_line("/proc/stat", "cpu", buf);
    sscanf(buf, "cpu %"SCNu64" %"SCNu64" %"SCNu64" %"SCNu64"\n", &usage.user_ticks, &usage.system_ticks, &usage.idle_ticks, &usage.nice_ticks);
    return usage;
}

struct mem_usage get_mem_usage() {
    struct mem_usage usage;
    char buf[1234];

    read_proc_line("/proc/meminfo", "MemTotal:", buf);
    sscanf(buf, "MemTotal: %"PRIu64" kB\n", &usage.total);
    
    read_proc_line("/proc/meminfo", "MemFree:", buf);
    sscanf(buf, "MemFree: %"PRIu64" kB\n", &usage.free);
    
    read_proc_line("/proc/meminfo", "MemAvailable:", buf);
    sscanf(buf, "MemAvailable: %"PRIu64" kB\n", &usage.available);
    
    read_proc_line("/proc/meminfo", "Cached:", buf);
    sscanf(buf, "Cached: %"PRIu64" kB\n", &usage.cached);
    
    read_proc_line("/proc/meminfo", "Active:", buf);
    sscanf(buf, "Active: %"PRIu64" kB\n", &usage.active);
    
    read_proc_line("/proc/meminfo", "Inactive:", buf);
    sscanf(buf, "Inactive: %"PRIu64" kB\n", &usage.inactive);
    
    read_proc_line("/proc/meminfo", "Swapins:", buf);
    sscanf(buf, "Swapins: %"PRIu64" kB\n", &usage.swapins);
 
    read_proc_line("/proc/meminfo", "Swapouts:", buf);
    sscanf(buf, "Swapouts: %"PRIu64" kB\n", &usage.swapouts);

    read_proc_line("/proc/meminfo", "Wirecount:", buf);
    sscanf(buf, "Wirecount: %"PRIu64" kB\n", &usage.wirecount);
    
    return usage;
}

struct uptime_info get_uptime() {
    struct sysinfo info;
    sysinfo(&info);
    struct uptime_info uptime = {
        .uptime_ticks = info.uptime,
        .load_1m = info.loads[0],
        .load_5m = info.loads[1],
        .load_15m = info.loads[2],
    };
    return uptime;
}

int get_cpu_count() {
    return get_nprocs();
}

int get_per_cpu_usage(struct cpu_usage** cpus_usage) {
    char buf[1234];
    char cpu_title[8];
    int ncpu = get_cpu_count();
    struct cpu_usage* cpus_load_data = (struct cpu_usage*)calloc(ncpu, sizeof(struct cpu_usage));
    if (!cpus_load_data) {
        return -ENOMEM;
    }
    
    for (int i = 0; i < ncpu; i++) {
        int cpu_num;
        int title_len = snprintf(cpu_title, 8, "cpu%d", i);
        if (title_len > 8 || title_len < 0) {
            STRACE("Can't load info of cpu %d (>10000)", i);
            return -ENOMEM;
        }
        read_proc_line("/proc/stat", cpu_title, buf);
        sscanf(buf, "cpu%d %"SCNu64" %"SCNu64" %"SCNu64" %"SCNu64"\n", &cpu_num, &cpus_load_data[i].user_ticks, &cpus_load_data[i].system_ticks, &cpus_load_data[i].idle_ticks, &cpus_load_data[i].nice_ticks);
    }
    *cpus_usage = cpus_load_data;
    return 0;
}
