/*********************************************************************************
 * @file		ringbufer.hpp
 * @brief		ringbufer belongs to zltest
 * @details		ringbufer belongs to zltest
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

#ifndef RINGBUFFER_RINGBUFER_HPP_
#define RINGBUFFER_RINGBUFER_HPP_
#include <string.h>

#define RING_BUFFER_VERSION     1
#define RING_BUFFER_INITED  0x55AA
#define RING_BUFFER_SIZE    (1000*50)

struct RingBufferHead
{
    int version;
    int inited;
    int write_index;
    int read_index;
    int head_size;
    int buf_size;
    int write_packet;
    int drop_packet;
    char module_name[20];
};

struct RingBufferInfo
{
    RingBufferHead head;
    int res[100 - sizeof(RingBufferHead)];
    char buf[0];
};

class RingBuffer
{
public:
    RingBuffer(int buf_size);
    virtual ~RingBuffer() = 0;
    bool init(std::string module_name, char *ptr);
    void print_head(int detail = 1);
    bool is_ready();
    bool is_empty();
    bool is_full();
    int write_buf(char *data, int data_len);
    int read_buf(char *data, int max_data_len);
    int get_size();
    int get_cnt();
    int get_free();
    void clear_statistics(void);
private:
    bool init_head(std::string module_name);
    int next_write(int index);
    int next_read(int index);
    RingBufferInfo *head_ptr;
    int init_buf_size;
};

class RingBufferShm : public RingBuffer
{
public:
    RingBufferShm(int buf_size);
    ~RingBufferShm();
    bool init(std::string module_name, std::string path, int id);
private:
    std::string path;
    int id;
    int init_buf_size;
    char *shmptr;
};



#endif /* RINGBUFFER_RINGBUFER_HPP_ */
