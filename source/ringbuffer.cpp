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

RingBufferShm::RingBufferShm(int buf_size):
RingBuffer(buf_size), path(""), id(0), init_buf_size(buf_size), shmptr(NULL)
{
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
    return RingBuffer::init(module_name, shmptr);
}

RingBuffer::RingBuffer(int buf_size)
{
    head_ptr = NULL;
    this->init_buf_size = buf_size;
}

RingBuffer::~RingBuffer()
{
    ;
}

bool RingBuffer::init(std::string module_name, char *ptr)
{
    head_ptr = (RingBufferInfo *)ptr;
    (void)init_head(module_name);
    return true;
}

bool RingBuffer::init_head(std::string module_name)
{
    if ((head_ptr->head.inited == RING_BUFFER_INITED)
            && (head_ptr->head.version == RING_BUFFER_VERSION)
            && (head_ptr->head.buf_size == init_buf_size)
            && (module_name == std::string(head_ptr->head.module_name)))
    {
        std::cout<<"module " << module_name << " version " <<head_ptr->head.version<<"already inited"<<std::endl;
        return true;
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
    return head_ptr->head.read_index == next_write(1);
}

static void data_to_buffer(char *buf, char *data, int data_len, int index, int size)
{
    if ((index + data_len) <= size)
    {
        memcpy(&buf[index], data, data_len);
    }
    else
    {
        memcpy(&buf[index], data, size - index);
        memcpy(&buf[0], &data[size - index], data_len + index - size);
    }
    return;
}

int RingBuffer::write_buf(char *data, int data_len)
{
    int free_buf = get_free();
    int tmp_write_index = head_ptr->head.write_index;
    if (data_len > free_buf)
    {
        head_ptr->head.drop_packet++;
        return -1;
    }

    if (!is_ready()) return -1;
    data_to_buffer(head_ptr->buf, data, data_len, tmp_write_index, get_size());
    tmp_write_index = next_write(data_len);
    if (!is_ready()) return -1;
    head_ptr->head.write_index = tmp_write_index;
    head_ptr->head.write_packet++;
    return data_len;
}

static void buffer_to_data(char *buf, char *data, int data_len, int index, int size)
{
    if ((index + data_len) <= size)
    {
        memcpy(data, &buf[index], data_len);
    }
    else
    {
        memcpy(data, &buf[index], size - index);
        memcpy(&data[size - index], &buf[0], data_len + index - size);
    }
    return;
}

int RingBuffer::read_buf(char *data, int max_data_len)
{
    if (is_empty()) return 0;

    int cnt = get_cnt();
    int read_cnt = (cnt > max_data_len) ? max_data_len : cnt;
    int tmp_read_index = head_ptr->head.read_index;

    if (!is_ready()) return 0;
    buffer_to_data(head_ptr->buf, data, read_cnt, tmp_read_index, get_size());
    tmp_read_index = next_read(read_cnt);
    if (!is_ready()) return 0;

    head_ptr->head.read_index = tmp_read_index;
    return read_cnt;
}

int RingBuffer::get_size()
{
    return head_ptr->head.buf_size;
}
int RingBuffer::get_cnt()
{
    return (head_ptr->head.write_index + head_ptr->head.buf_size - head_ptr->head.read_index) % head_ptr->head.buf_size;
}

int RingBuffer::get_free()
{
    return (head_ptr->head.read_index + head_ptr->head.buf_size - head_ptr->head.write_index -1) % head_ptr->head.buf_size;
}

int RingBuffer::next_read(int index)
{
    return (head_ptr->head.read_index + index) % head_ptr->head.buf_size;
}

int RingBuffer::next_write(int index)
{
    return (head_ptr->head.write_index + index) % head_ptr->head.buf_size;
}
