#ifndef _DATA_QUEUE_LIB_H_
#define _DATA_QUEUE_LIB_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdlib.h>
#include "sdlist.h"

typedef void *DQ_HANDLE;

typedef enum
{
    CLOUD_DATA_TYPE_UNKNOW = 0,

    CLOUD_DATA_TYPE_TS_PACKET,
    CLOUD_DATA_TYPE_PES_DATA,
    CLOUD_DATA_TYPE_SETION_DATA,

    CLOUD_DATA_TYPE_ES_H264,
    CLOUD_DATA_TYPE_ES_MP3,
    CLOUD_DATA_TYPE_ES_AAC,

    CLOUD_DATA_TYPE_IP,
    CLOUD_DATA_TYPE_UDP,
    CLOUD_DATA_TYPE_RTP,

    CLOUD_DATA_TYPE_AUDIO_PCM,
    CLOUD_DATA_TYPE_VIDEO_DATA
} CLOUD_DATA_TYPE;

typedef struct _CLOUD_DATA_LIST_
{
    K_dlist_entry list_entry;

    CLOUD_DATA_TYPE cloud_data_type;
    unsigned char *pdata_buffer;
    unsigned int u32data_length;
    void *puser_data;
} CLOUD_DATA_LIST;

/**
 * @brief Callback for foreach of the queue
 *
 * @param node Current CLOUD_DATA_LIST node
 * @param args Parameters
 * set for data_queue_foreach()
 *
 * @return 0 for continue, -1 for quit from for each loop
 */
typedef int (*DQ_FOREACH_CBK) (CLOUD_DATA_LIST * node, void *args);

typedef void (*DQ_CLEAR_NODE_CBK) (CLOUD_DATA_LIST ** ppNode);

DQ_HANDLE data_queue_create(void);
int data_queue_destroy(DQ_HANDLE h);

int data_queue_size(DQ_HANDLE h);

int data_queue_is_empty(DQ_HANDLE h);
void data_queue_push_head(DQ_HANDLE h, CLOUD_DATA_LIST * node);
void data_queue_push_back(DQ_HANDLE h, CLOUD_DATA_LIST * node);
void data_queue_remove_entry(DQ_HANDLE h, CLOUD_DATA_LIST * node);
CLOUD_DATA_LIST *data_queue_pop_head(DQ_HANDLE h);
CLOUD_DATA_LIST *data_queue_pop_back(DQ_HANDLE h);
CLOUD_DATA_LIST *data_queue_foreach(DQ_HANDLE h, DQ_FOREACH_CBK foreach_fun, void *args);
void data_queue_clear(DQ_HANDLE h, DQ_CLEAR_NODE_CBK clear_fun);

#ifdef __cplusplus
}                               /* extern "C" */
#endif

#endif /* _DATA_QUEUE_LIB_H_ */
