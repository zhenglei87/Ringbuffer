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


using UINT32 = unsigned int;

struct RingBufferHead
{
    int version;
    int inited;
    UINT32 write_index;
    UINT32 read_index;
    UINT32 head_size;
    UINT32 buf_size;
    UINT32 write_packet;
    UINT32 drop_packet;
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
    RingBuffer();
    virtual ~RingBuffer() = 0;
    bool init(std::string module_name, char *ptr, UINT32 buf_size);
    void print_head(int detail = 1);
    bool is_ready();
    bool is_empty();
    bool is_full();
    UINT32 write_buf(char *data, UINT32 data_len);
    UINT32 read_buf(char *data, UINT32 max_data_len);
    UINT32 get_size();
    UINT32 get_cnt();
    UINT32 get_free();
    void clear_statistics(void);
private:
    bool init_head(std::string module_name);
    RingBufferInfo *head_ptr;
    UINT32 init_buf_size;
};

class RingBufferShm : public RingBuffer
{
public:
    RingBufferShm(UINT32 buf_size);
    ~RingBufferShm();
    bool init(std::string module_name, std::string path, int id);
private:
    std::string path;
    int id;
    UINT32 init_buf_size;
    char *shmptr;
};



#endif /* RINGBUFFER_RINGBUFER_HPP_ */
