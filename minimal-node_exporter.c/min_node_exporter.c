#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define PORT 9100
#define BUF_SIZE 4096

//cpu usage calculation in percentage
double get_cpu_usage() {
    static long prev_idle = 0, prev_total = 0;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return -1;
    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);

    long idle_time = idle + iowait;
    long total_time = user + nice + system + idle + iowait + irq + softirq + steal;

    long diff_idle = idle_time - prev_idle;
    long diff_total = total_time - prev_total;

    prev_idle = idle_time;
    prev_total = total_time;

    if (diff_total == 0) return 0.0;
    return (1.0 - (double)diff_idle / diff_total) * 100.0;
}

//ram usage in percentage
double get_memory_usage() {
    long mem_total, mem_free, buffers, cached;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;
    fscanf(fp, "MemTotal: %ld kB\n", &mem_total);
    fscanf(fp, "MemFree: %ld kB\n", &mem_free);
    fscanf(fp, "Buffers: %ld kB\n", &buffers);
    fscanf(fp, "Cached: %ld kB\n", &cached);
    fclose(fp);

    long used = mem_total - (mem_free + buffers + cached);
    return (double)used / mem_total * 100.0;
}

//disk usage in percentage
double get_disk_usage(const char *path) {
    struct statvfs stat;
    if (statvfs(path, &stat) != 0) return -1;
    unsigned long total = stat.f_blocks * stat.f_frsize;
    unsigned long free = stat.f_bfree * stat.f_frsize;
    unsigned long used = total - free;
    return (double)used / total * 100.0;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char buffer[BUF_SIZE];
    char response[BUF_SIZE];

    //socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Exporter running on http://0.0.0.0:%d/metrics\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        read(new_socket, buffer, BUF_SIZE);

        //metrics
        double cpu = get_cpu_usage();
        double mem = get_memory_usage();
        double disk = get_disk_usage("/");

        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n\r\n"
                 "# HELP cpu_usage CPU usage in percent\n"
                 "# TYPE cpu_usage gauge\n"
                 "cpu_usage %.2f\n"
                 "# HELP memory_usage RAM usage in percent\n"
                 "# TYPE memory_usage gauge\n"
                 "memory_usage %.2f\n"
                 "# HELP disk_usage Disk usage in percent\n"
                 "# TYPE disk_usage gauge\n"
                 "disk_usage %.2f\n",
                 cpu, mem, disk);

        send(new_socket, response, strlen(response), 0);
        close(new_socket);

        sleep(1); //avoid frequent requests
    }

    return 0;
}
