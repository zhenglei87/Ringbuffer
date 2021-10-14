/*********************************************************************************
 * @file		ringbuffer.cpp
 * @brief		ringbuffer belongs to zltest
 * @details		ringbuffer belongs to zltest
 * @author		zhenglei
 * @date		2021/09/13
 * @copyright	Copyright (c) 2021 Gohigh V2X Division.
 * @verbatim
 *
 *  Change History:
 *  Date      Author    Version  ChangeId           Description
 *  ------------------------------------------------------------------------------
 *  2021/09/13 zhenglei       1.0       ————             Create this file
 *
 * @endverbatim
 ********************************************************************************/
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <assert.h>
#include "ringbuffer.hpp"
#include<errno.h>

RingBufferShm::RingBufferShm(UINT32 buf_size):
RingBuffer(), path(""), id(0), init_buf_size(buf_size), shmptr(NULL)
{
    while (buf_size & (buf_size -1))
    {
        buf_size = buf_size & (buf_size -1);
    }
    init_buf_size = buf_size << 1;//roundup pow of two, 简化取模运算
}

RingBufferShm::~RingBufferShm()
{
    shmdt(shmptr);
    return;
}

extern int errno;

bool RingBufferShm::init(std::string module_name, std::string path, int id)
{
    this->path = path;
    this->id = id;

    key_t key = ftok(path.c_str(), id);
    assert(key > 0);

    int shmid = shmget(key, init_buf_size + sizeof(RingBufferInfo), IPC_CREAT);
    std::cout << "error = " << errno << " shmid = "<< shmid << std::endl;
    assert(shmid >= 0);

    shmptr = (char *)shmat(shmid, 0, 0);
    assert(shmptr != NULL);
    return RingBuffer::init(module_name, shmptr, init_buf_size);
}

RingBuffer::RingBuffer()
{
    head_ptr = NULL;
    this->init_buf_size = 0;
}

RingBuffer::~RingBuffer()
{
    ;
}

bool RingBuffer::init(std::string module_name, char *ptr, UINT32 buf_size)
{
    head_ptr = (RingBufferInfo *)ptr;
    this->init_buf_size = buf_size;
    (void)init_head(module_name);
    return true;
}

bool RingBuffer::init_head(std::string module_name)
{
    if (head_ptr->head.inited == RING_BUFFER_INITED)
    {
        if ((head_ptr->head.version != RING_BUFFER_VERSION)
                || (head_ptr->head.buf_size != init_buf_size)
                || (module_name != std::string(head_ptr->head.module_name)))
        {
            std::cout<<"shm module " << head_ptr->head.module_name << " version " <<head_ptr->head.version<<" size "<<head_ptr->head.buf_size<<" already inited"<<std::endl;
            std::cout<<"init module " << module_name << " version "<<RING_BUFFER_VERSION<< " size "<< init_buf_size <<" already inited"<<std::endl;
            return false;
        }
        else
        {
            return true;
        }
    }

    memset(head_ptr, 0, sizeof(RingBufferHead));
    head_ptr->head.version = RING_BUFFER_VERSION;
    head_ptr->head.read_index = 0;
    head_ptr->head.write_index = 0;
    head_ptr->head.head_size = sizeof(RingBufferHead);
    head_ptr->head.buf_size = init_buf_size;
    module_name.copy(head_ptr->head.module_name, 20, 0);
    head_ptr->head.inited = RING_BUFFER_INITED;
    return true;
}

void RingBuffer::clear_statistics(void)
{
    head_ptr->head.write_packet = 0;
    head_ptr->head.drop_packet = 0;
    return;
}

void RingBuffer::print_head(int detail)
{
    if (1 == detail)
    {
        std::cout<<"version = "<<head_ptr->head.version<<std::endl;
        std::cout<<"name = "<<head_ptr->head.module_name<<std::endl;
        std::cout<<"inited = "<<head_ptr->head.inited<<std::endl;
        std::cout<<"read index = "<<head_ptr->head.read_index<<std::endl;
        std::cout<<"write index = "<<head_ptr->head.write_index<<std::endl;
        std::cout<<"head size = "<<head_ptr->head.head_size<<std::endl;
        std::cout<<"buffer size = "<<head_ptr->head.buf_size<<std::endl;
        std::cout<<"write packet = "<<head_ptr->head.write_packet<<std::endl;
        std::cout<<"drop packet = "<<head_ptr->head.drop_packet<<std::endl;
    }
    else
    {
        printf("wp = %i, dp = %i, cnt = %i\n",
                head_ptr->head.write_packet,
                head_ptr->head.drop_packet,
                get_cnt());
    }
}

bool RingBuffer::is_ready()
{
    return (RING_BUFFER_INITED == head_ptr->head.inited);
}

bool RingBuffer::is_empty()
{
    return head_ptr->head.read_index == head_ptr->head.write_index;
}
bool RingBuffer::is_full()
{
    return ((head_ptr->head.read_index - head_ptr->head.write_index) & (head_ptr->head.buf_size -1)) == 0;
}

static void data_to_buffer(char *buf, char *data, int data_len, int index, int size)
{
    auto l = std::min(data_len, size - index);
    memcpy(&buf[index], data, l);
    memcpy(&buf[0], data + l, data_len - l);
    return;
}

UINT32 RingBuffer::write_buf(char *data, UINT32 data_len)
{
    UINT32 free_buf = get_free();
    if (data_len > free_buf)
    {
        head_ptr->head.drop_packet++;
        return -1;
    }
    data_to_buffer(head_ptr->buf, data, data_len,
            head_ptr->head.write_index & (head_ptr->head.buf_size - 1), get_size());
    head_ptr->head.write_index += data_len;
    head_ptr->head.write_packet++;
    return data_len;
}

static void buffer_to_data(char *buf, char *data, UINT32 data_len, UINT32 index, UINT32 size)
{
    auto l = std::min(data_len, size - index);
    memcpy(data, &buf[index], l);
    memcpy(&data[l], &buf[0], data_len - l);
    return;
}

UINT32 RingBuffer::read_buf(char *data, UINT32 max_data_len)
{
    UINT32 cnt = get_cnt();
    UINT32 read_cnt = std::min(max_data_len, cnt);
    buffer_to_data(head_ptr->buf, data, read_cnt, head_ptr->head.read_index & (head_ptr->head.buf_size - 1), get_size());
    head_ptr->head.read_index += read_cnt;
    return read_cnt;
}

UINT32 RingBuffer::get_size()
{
    return head_ptr->head.buf_size;
}
UINT32 RingBuffer::get_cnt()
{
    return (head_ptr->head.write_index - head_ptr->head.read_index);
}

UINT32 RingBuffer::get_free()
{
    return (head_ptr->head.read_index + head_ptr->head.buf_size - head_ptr->head.write_index);
}
