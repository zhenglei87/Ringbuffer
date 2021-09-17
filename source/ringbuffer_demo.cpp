/*********************************************************************************
 * @file		ringbuffer_demo.cpp
 * @brief		ringbuffer_demo belongs to zltest
 * @details		ringbuffer_demo belongs to zltest
 * @author		zhenglei
 * @date		2021/09/16
 * @copyright	Copyright (c) 2021 Gohigh V2X Division.
 * @verbatim
 *
 *  Change History:
 *  Date      Author    Version  ChangeId           Description
 *  ------------------------------------------------------------------------------
 *  2021/09/16 zhenglei       1.0       ————             Create this file
 *
 * @endverbatim
 ********************************************************************************/
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "asyncwork.hpp"
#include "ringbuffer.hpp"

class CTimer
{
public:
    CTimer()
    {
        return;
    }
    ~CTimer()
    {
        return;
    }
    void tick_start(void)
    {
        gettimeofday(&startTV,NULL);
        return;
    }
    long get_us(void)
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return (tv.tv_sec - startTV.tv_sec) * 1000000 + tv.tv_usec - startTV.tv_usec;
    }
private:
    struct timeval startTV;
};

long write_proc(RingBuffer &buf, int period_ms, int cnt, int times)
{
    char tmp[100];
    int write_cnt = 0;
    CTimer timer;
    long time_sum = 0;
    long time = 0;
    std::cout<<"period_ms = "<<period_ms<<" cnt ="<<cnt<<" times = "<<times<<std::endl;

    timer.tick_start();
    for (int i = 0; i < times; i++)
    {
        timer.tick_start();
        for (int j = 0; j < cnt; j++)
        {
            write_cnt = snprintf(tmp, 99, "write_thread %08ld\n", time);
            buf.write_buf(tmp, write_cnt);
        }
        time = timer.get_us();
        time_sum +=time;
        if (time < period_ms * 1000)
        {
            usleep(period_ms * 1000 - time);
        }
    }
    return time_sum;
}

long print_proc(RingBuffer &buf, int period_ms, int times, FILE *fd)
{
    char tmp[1024];
    int read_cnt;
    long read_ret = 0;
    CTimer timer;
    long time_sum = 0;
    long time = 0;
    int rate_limite = 200 * period_ms / 1000; //rate limite 200 KB/s

    printf("period_ms = %i, times = %i, rate_limite = %i\n", period_ms, times, rate_limite * 1000);

    for (int i = 0; i < times; i++)
    {
        timer.tick_start();
        std::cout<<"time = "<<time<<" ";
        buf.print_head(0);
        for (int j = 0; j < rate_limite; j++)
        {
            read_cnt = buf.read_buf(tmp, 1000);
            if (read_cnt > 0)
            {
                read_ret += read_cnt;
                if (fd != NULL)
                {
                    fwrite(tmp, 1, read_cnt, fd);
                }
            }
            else
            {
                break;
            }
        }
        time = timer.get_us();
        time_sum += time;
        usleep(period_ms * 1000 - time);
    }
    return time_sum;
}

int main(int argc, char *argv[])
{
    if ((argc == 5) && (*argv[1] == 'r'))
    {
        int peroid = std::stoi(argv[2]);
        int times = std::stoi(argv[3]);
        std::string mod(argv[4]);
        int read_times = 0;
        RingBufferShm ringbuf_r(RING_BUFFER_SIZE);
        FILE *fd = NULL;

        if (mod == "file")
        {
            fd = fopen("/root/zltest.log", "wb+");
        }
        else if (mod == "screen")
        {
            fd = stdout;
        }

        ringbuf_r.init("test", "/root/shm", 0);
        ringbuf_r.print_head();
        read_times = print_proc(ringbuf_r, peroid, times, fd);
        printf("read proc times = %i\n", read_times);
    }
    else if ((argc == 5) && (*argv[1] == 'w'))
    {
        int peroid = std::stoi(argv[2]);
        int cnt = std::stoi(argv[3]);
        int times = std::stoi(argv[4]);
        int write_times = 0;
        RingBufferShm ringbuf_w(RING_BUFFER_SIZE);

        ringbuf_w.init("test", "/root/shm", 0);
        ringbuf_w.clear_statistics();
        ringbuf_w.print_head();
        write_times = write_proc(ringbuf_w, peroid, cnt, times);
        printf("write proc times = %i\n", write_times);
    }
    {
        printf("USAGE:\n");
        printf("%s r 100 20\n", argv[0]);
        printf("%s w 10 100 100\n", argv[0]);
    }
    return 0;
}



