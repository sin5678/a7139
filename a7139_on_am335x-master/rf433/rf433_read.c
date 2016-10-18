#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "common.h"
#include "rf433lib.h"

#define USAGE           "-o <r|w|rw|wr|rw2|c|dump> [-n netid] [-w wfreq] [-r rate] [-l loop] [-t delay]"
#define DEV             "/dev/a7139-1"

#define SE433_SET_NETID 0x01
#define SE433_SET_WFREQ 0x02
#define SE433_SET_RATE  0x04

uint16_t se433_netid = 0;
uint8_t se433_wfreq = 0;
uint8_t se433_rate = 0;
pthread_t r_tid, w_tid;
time_t start_t, end_t;

int fd = -1;
int delay = 10;
char *op = NULL;
unsigned long r_err = 0;
unsigned long w_err = 0;
unsigned long r_cnt = 0;
unsigned long w_cnt = 0;
unsigned long cnt;
unsigned long loop = 0;


void *rf433_read_job(void *p)
{
    int ret, i;
    char buf[64];
    unsigned long r_cnt = 0;

    while (1) {
        ret = read(fd, buf, 64);
        if (ret <= 0) {
            printf("read error %s\n", strerror(errno));
            //pthread_exit((void*)-1);
        }
        r_cnt++;

        printf("\nr_cnt=%lu\n", r_cnt);

        for (i = 0; i < ret; i++) {
            printf("%02x ", buf[i]);
            if ((i+1) % 16 == 0) {
                printf("\n");
            }
        }
    }
    pthread_exit((void*)0);
}

void *rf433_write_job(void *p)
{
    int ret, i;
    char buf[64];
    unsigned long w_cnt = 0;

    while (1) {
        memset(buf, (char)(w_cnt%256), sizeof(buf));

        ret = write(fd, buf, 64);
        if (ret <= 0) {
            printf("write error %s\n", strerror(errno));
            //pthread_exit((void*)-1);
        }
        w_cnt++;

        printf("    w_cnt=%lu\n", w_cnt);
        printf("    ");
        for (i = 0; i < 64; i++) {
            printf("%02x ", buf[i]);
            if ((i+1) % 16 == 0) {
                printf("\n    ");
            }
        }

        //nanosleep(&tdely, NULL);
        usleep(delay * 1000);
    }
    pthread_exit((void*)0);
}

void rf433_sigintterm(int UNUSED(unused))
{
    end_t = time(NULL);
    usleep(50 * 1000);

    printf("\n\n");

    if (op && (strcmp(op, "w") == 0)) {
        printf("+++ total use %ld seconds.\n", end_t - start_t);
        printf("+++ w_cnt %lu, w_err %lu\n", w_cnt, w_err);
        printf("+++ average %.3f packet peer second.\n", (float)w_cnt/(end_t - start_t));
        printf("+++ packet error rate %.5f\n", (float)w_err/(w_cnt + w_err));
    }
    else if (op && (strcmp(op, "r") == 0)) {
        printf("+++ total read use %ld seconds.\n", end_t - start_t);
        printf("+++ r_cnt %lu, r_err %lu\n", r_cnt, r_err);
        printf("+++ average %.3f packet peer second.\n", (float)r_cnt/(end_t - start_t));
        printf("+++ packet error rate %.5f\n", (float)r_err/(r_cnt + r_err));
    }

    printf("+++ finished.\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int ret, opt, i;
    char buf[64];
    uint8_t se433_set = 0;

    if (argc < 2) {
        fprintf(stderr, "%s: %s\n", argv[0], USAGE);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "o:n:w:r:l:t:")) != -1) {
        switch (opt) {
            case 'o':
                op = optarg;
                break;

            case 'n':
                se433_set |= SE433_SET_NETID;
                se433_netid = RF433_NETID(atoi(optarg));
                break;

            case 'w':
                se433_set |= SE433_SET_WFREQ;
                se433_wfreq = atoi(optarg);
                break;

            case 'r':
                se433_set |= SE433_SET_RATE;
                se433_rate = atoi(optarg);
                break;

            case 'l':
                loop = atoi(optarg);
                break;

            case 't':
                delay = atoi(optarg);
                break;

            default: /* '?' */
                fprintf(stderr, "%s: %s\n", argv[0], USAGE);
                exit(EXIT_FAILURE);
        }
    }

    if (op == NULL) {
        fprintf(stderr, "-o operation must be set!\n");
        exit(-1);
    }

    if (signal(SIGINT, rf433_sigintterm) == SIG_ERR ||
            signal(SIGTERM, rf433_sigintterm) == SIG_ERR ||
            signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "signal(rf433_sigintterm) error\n");
        exit(-1);
    }

    fd = open(DEV, O_RDWR);
    if (fd < 0) {
        printf("open %s error: %s\n", DEV, strerror(errno));
        exit(-1);
    }

    if (se433_set & SE433_SET_NETID) {
        printf("set %s netid 0x%x\n", DEV, se433_netid);
        ret = rf433_set_netid(fd, se433_netid);
        if (ret < 0) {
            fprintf(stderr, "rf433_set_netid error: %d, %s\n", ret, strerror(errno));
        }

        se433_netid = 0;
        ret = rf433_get_netid(fd, &se433_netid);
        if (ret < 0) {
            fprintf(stderr, "rf433_get_netid error: %d, %s\n", ret, strerror(errno));
        }
        printf("get %s netid is 0x%x\n", DEV, se433_netid);
    }

    if (se433_set & SE433_SET_WFREQ) {
        printf("set %s wfreq %d, %s\n", DEV, se433_wfreq, rf433_get_freq_str(se433_wfreq));
        rf433_set_wfreq(fd, se433_wfreq);
        if (ret < 0) {
            fprintf(stderr, "se433_rate error: %d\n", ret);
        }

        se433_wfreq = 0;
        ret = rf433_get_wfreq(fd, &se433_wfreq);
        if (ret < 0) {
            fprintf(stderr, "rf433_get_wfreq error: %d\n", ret);
        }
        printf("get %s wfreq %d, %s\n", DEV, se433_wfreq, rf433_get_freq_str(se433_wfreq));
    }

    if (se433_set & SE433_SET_RATE) {
        printf("set %s rate %d, %s\n", DEV, se433_rate, rf433_get_rate_str(se433_rate));
        rf433_set_rate(fd, se433_rate);
        if (ret < 0) {
            fprintf(stderr, "rf433_set_rate error: %d\n", ret);
        }

        se433_rate = 0;
        ret = rf433_get_rate(fd, &se433_rate);
        if (ret < 0) {
            fprintf(stderr, "rf433_get_rate error: %d\n", ret);
        }
        printf("get %s rate %d, %s\n", DEV, se433_rate, rf433_get_rate_str(se433_rate));
    }

    if (loop) {
        printf("loop=%ld\n", loop);
        cnt = loop;
    } else {
        cnt = 1;
    }
    printf("delay=%d(ms)\n", delay);

    if (strcmp(op, "rw") == 0) {
        while (cnt) {
            ret = read(fd, buf, 64);
            if (ret <= 0) {
                printf("(%lu)read error %s\n", ++r_err, strerror(errno));
            } else {
                r_cnt++;
            }
#if 0
            for (i = 0; i < ret; i++) {
                printf("%02x ", buf[i]);
                if ((i+1) % 16 == 0) {
                    printf("\n");
                }
            }
#endif
            ret = write(fd, buf, 64);
            if (ret <= 0) {
                printf("(%lu)write error %s\n", ++w_err, strerror(errno));
            } else {
                w_cnt++;
            }

            printf("r_cnt=%lu, w_cnt=%lu\n", r_cnt, w_cnt);
            printf("r_err=%lu, w_err=%lu\n", r_err, w_err);

            if (loop) {
                cnt--;
            }
        }
    }

    else if (strcmp(op, "wr") == 0){
        while (cnt) {
            ret = write(fd, buf, 64);
            if (ret <= 0) {
                printf("(%lu)read error %s\n", ++r_err, strerror(errno));
            } else {
                w_cnt++;
            }

            ret = read(fd, buf, 64);
            if (ret <= 0) {
                printf("(%lu)write error %s\n", ++w_err, strerror(errno));
            } else {
                r_cnt++;
            }

            printf("w_cnt=%lu, r_cnt=%lu\n", w_cnt, r_cnt);
            printf("r_err=%lu, w_err=%lu\n", r_err, w_err);

            if (loop) {
                cnt--;
            }
        }
    }

    else if (strcmp(op, "r") == 0) {

        start_t = time(NULL);

        while (cnt) {
            ret = read(fd, buf, 64);
            if (ret <= 0) {
                printf("(%lu)read error %s\n", ++r_err, strerror(errno));
            } else {
                r_cnt++;
            }

            printf("r_cnt=%lu\n", r_cnt);

            for (i = 0; i < ret; i++) {
                printf("%02x ", buf[i]);
                if ((i+1) % 16 == 0) {
                    printf("\n");
                }
            }

            if (loop) {
                cnt--;
            }
        }

        end_t = time(NULL);

        printf("+++ total read use %ld seconds.\n", end_t - start_t);
        printf("+++ r_cnt %lu, r_err %lu\n", r_cnt, r_err);
        printf("+++ average %.3f packet peer second.\n", (float)w_cnt/(end_t - start_t));
        printf("+++ packet error rate %.5f\n", (float)r_err/(r_cnt + r_err));
    }

    else if (strcmp(op, "w") == 0) {

        start_t = time(NULL);

        while (cnt) {
            memset(buf, (char)(w_cnt%256), sizeof(buf));
            for (i = 0; i < 64; i++) {
                printf("%02x ", buf[i]);
                if ((i+1) % 16 == 0) {
                    printf("\n");
                }
            }

            ret = write(fd, buf, 64);
            if (ret <= 0) {
                printf("write error %s\n", strerror(errno));
                w_err++;
            } else {
                w_cnt++;
            }

            //nanosleep(&tdely, NULL);
            usleep(delay * 1000);

            printf("w_cnt=%lu\n", w_cnt);

            if (loop) {
                cnt--;
            }
        }

        end_t = time(NULL);

        printf("+++ total write use %ld seconds.\n", end_t - start_t);
        printf("+++ w_cnt %lu, w_err %lu\n", w_cnt, w_err);
        printf("+++ average %.3f packet peer second.\n", (float)w_cnt/(end_t - start_t));
        printf("+++ packet error rate %.5f\n", (float)w_err/(w_cnt + w_err));
    }

    else if (strcmp(op, "rw2") == 0) {

        ret = pthread_create(&r_tid, NULL, rf433_read_job, 0);
        if (ret == -1) {
            fprintf(stderr, "read thread create faild!");
            goto out;
        }

        ret = pthread_create(&w_tid, NULL, rf433_write_job, 0);
        if (ret == -1) {
            fprintf(stderr, "write thread create faild!");
            goto out;
        }

        pthread_join(r_tid, NULL);
        pthread_join(w_tid, NULL);
    }

    else if (strcmp(op, "dump") == 0) {
        se433_netid = 0;
        ret = rf433_get_netid(fd, &se433_netid);
        if (ret < 0) {
            fprintf(stderr, "rf433_get_netid error: %d, %s\n", ret, strerror(errno));
        }
        printf("get %s netid is 0x%x\n", DEV, se433_netid);

        se433_wfreq = 0;
        ret = rf433_get_wfreq(fd, &se433_wfreq);
        if (ret < 0) {
            fprintf(stderr, "rf433_get_wfreq error: %d, %s\n", ret, strerror(errno));
        }
        printf("get %s wfreq is %d\n", DEV, se433_wfreq);

        se433_rate = 0;
        ret = rf433_get_rate(fd, &se433_rate);
        if (ret < 0) {
            fprintf(stderr, "rf433_get_rate error: %d, %s\n", ret, strerror(errno));
        }
        printf("get %s rate is %d\n", DEV, se433_rate);
    }

out:
    close(fd);
    exit(ret);
}

