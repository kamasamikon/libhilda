/*
 * parsetxt.c
 *
 *  Created on: Aug 12, 2010
 *      Author: pete
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "sdlist.h"
#include "dataqueue.h"
#include "xtcool.h"
#include "parsetext.h"

#define MAX_LINE_LENGTH              512

typedef enum
{
    LINE_STR_TYPE_NULL = 0,
    LINE_STR_TYPE_TAG,
    LINE_STR_TYPE_CONTENT,
    LINE_STR_TYPE_EROOR
} S_LINE_STR_TYPE;

typedef struct _PARSE_TXT_HANDLE_
{

    DQ_HANDLE line_string_queue;

    char *pLine_str;
    char *pTag_str;
    char *pNode_str;
    char *pValue_str;
    char *pTemp_str;

    unsigned char run_flag;
    PT_TAG_CBK Tag_cbk;
    void *pTag_userdata;
    PT_NOED_CBK Node_cbk;
    void *pNode_userdata;

} PARSE_TXT_HANDLE;

static void load_line_string(char *f_pFile_path_name, DQ_HANDLE f_str_queue)
{
    FILE *l_pRegFile = NULL;
    CLOUD_DATA_LIST *l_pNode = NULL;
    char *l_pStr = NULL;
    char *l_pLingBuffer = NULL;

    l_pLingBuffer = kmem_alloz(MAX_LINE_LENGTH, char);
    l_pRegFile = fopen(f_pFile_path_name, "r");

    if (l_pRegFile != NULL)
    {
        printf("load_line_string ::open the  %s file success \n  ", f_pFile_path_name);

        while (!feof(l_pRegFile))
        {
            l_pLingBuffer[0] = 0;
            l_pStr = NULL;
            l_pStr = fgets(l_pLingBuffer, MAX_LINE_LENGTH, l_pRegFile);

            if (l_pStr == NULL)
            {
                printf("load_line_string ::  file end \n  ");
                break;
            }

            if (strlen(l_pLingBuffer) > 2)
            {
                l_pStr = NULL;
                l_pStr = strdup(l_pLingBuffer);

                l_pNode = kmem_alloz(1, CLOUD_DATA_LIST);
                l_pNode->puser_data = l_pStr;
                data_queue_push_back(f_str_queue, l_pNode);
            }

        }

        fclose(l_pRegFile);

    }
    else
    {
        printf("load_line_string :: open the %s file  failed \n  ", f_pFile_path_name);
    }

    kmem_free_sz(l_pLingBuffer);
}

static void clear_str_node_cbk(CLOUD_DATA_LIST ** ppNode)
{
    CLOUD_DATA_LIST *l_pNode = *ppNode;

    *ppNode = NULL;

    if (l_pNode != NULL)
    {
        if (l_pNode->puser_data != NULL)
        {
            kmem_free_sz(l_pNode->puser_data);
        }

        kmem_free_sz(l_pNode);
    }

}

static void clear_handle(PARSE_TXT_HANDLE * h)
{
    if (h != NULL)
    {
        kmem_free_sz(h->pNode_str);
        kmem_free_sz(h->pTag_str);
        kmem_free_sz(h->pValue_str);
        kmem_free_sz(h->pLine_str);
        kmem_free_sz(h->pTemp_str);

        data_queue_clear(h->line_string_queue, clear_str_node_cbk);
        data_queue_destroy(h->line_string_queue);
        kmem_free_sz(h);
    }
}

/**
 * @brief        create parse txt instance
 * @param    iTxt_format              txt input format
 *                                                         0   ---> file format
 *                                                         1  ----> data buffer format
 * @param    f_pTxt_info                txt info
 *                                                         iTxt_format = 0,     f_pTxt_info is full path file name
 *                                                         iTxt_format = 1,        f_pTxt_info is txt string ,note: only support ASCII code
 *
 * @return     PT_HANDLE
 * @retval   no NULL                        success
 * @retval   NULL                            failled
 */
PT_HANDLE pt_parse_create(int iTxt_format, char *f_pTxt_info)
{

    PARSE_TXT_HANDLE *l_pH = NULL;

    if (iTxt_format != 0)
    {
        return NULL;            // at present , only support file path name
    }

    if (f_pTxt_info == NULL)
    {
        printf("pt_parse_create:: f_pTxt_info is NULL \n");
        return NULL;
    }

    if (access(f_pTxt_info, R_OK) != 0)
    {
        printf("pt_parse_create:: can't read the %s file \n", f_pTxt_info);
        return NULL;

    }

    l_pH = kmem_alloz(1, PARSE_TXT_HANDLE);
    assert(l_pH != NULL);

    l_pH->pLine_str = kmem_alloz(MAX_LINE_LENGTH, char);
    assert(l_pH->pLine_str != NULL);
    l_pH->pTag_str = kmem_alloz(MAX_LINE_LENGTH / 2, char);
    assert(l_pH->pTag_str != NULL);
    l_pH->pNode_str = kmem_alloz(MAX_LINE_LENGTH / 2, char);
    assert(l_pH->pNode_str != NULL);
    l_pH->pValue_str = kmem_alloz(MAX_LINE_LENGTH / 2, char);
    assert(l_pH->pValue_str != NULL);

    l_pH->pTemp_str = kmem_alloz(MAX_LINE_LENGTH / 2, char);
    assert(l_pH->pTemp_str != NULL);

    l_pH->line_string_queue = data_queue_create();

    load_line_string(f_pTxt_info, l_pH->line_string_queue);

    return l_pH;
}

static int is_tag(char *f_pStr)
{
    int l_iRet = 0;
    char *pdest0 = NULL;
    char *pdest1 = NULL;

    pdest0 = strstr(f_pStr, "[");
    pdest1 = strstr(f_pStr, "]");

    if (pdest0 != NULL && pdest1 != NULL && (pdest1 > pdest0))
    {
        l_iRet = 1;
        printf("is_tag:: %s  \n", f_pStr);
    }
    return l_iRet;

}

static S_LINE_STR_TYPE pop_line_string(DQ_HANDLE f_hLineStr, char *f_pLingBuffer)
{
    S_LINE_STR_TYPE l_type = LINE_STR_TYPE_NULL;
    CLOUD_DATA_LIST *l_pNode = NULL;
    char *l_pTempChar = NULL;

    l_pNode = data_queue_pop_head(f_hLineStr);

    if (l_pNode != NULL)
    {
        l_pTempChar = (char *)l_pNode->puser_data;

        if (l_pTempChar != NULL)
        {
            strcpy(f_pLingBuffer, l_pTempChar);
            kmem_free_sz(l_pTempChar);
            l_pTempChar = NULL;

            if (is_tag(f_pLingBuffer))
            {
                l_type = LINE_STR_TYPE_TAG;
            }
            else
            {
                l_type = LINE_STR_TYPE_CONTENT;
            }

        }
        else
        {
            l_type = LINE_STR_TYPE_EROOR;
            printf(" the node puser_data is NULL !!!!!!!!!!!!!!1  \n");
        }

        kmem_free_sz(l_pNode);
    }

    return l_type;
}

// head; remove  blanck
// end :remove blank and
static char *remove_tow_sides(PARSE_TXT_HANDLE * h, char *f_pStr)
{

    char *l_pTemp = h->pTemp_str;
    char *l_pStart = NULL;
    int i = 0;
    int l_iLen = strlen(f_pStr);

    memset(l_pTemp, 0, sizeof(l_pTemp));

    // head
    while (f_pStr[i] == ' ' || f_pStr[i] == '\t')
    {
        i++;
        if (i == l_iLen)
        {
            break;
        }
    }
    l_pStart = f_pStr + i;

    // end
    strcpy(l_pTemp, l_pStart);
    l_iLen = strlen(l_pTemp);
    i = 0;
    while (l_iLen >= 0)
    {
        if (l_pTemp[i] == ' ' || l_pTemp[i] == '\t' || l_pTemp[i] == '\n' || l_pTemp[i] == '\r')
        {
            break;
        }
        l_iLen--;
        i++;
    }

    l_pTemp[i] = 0;

    return l_pTemp;
}

static int split_noed_value_string_line(PARSE_TXT_HANDLE * h, char *f_pLineStr, char *f_pNode, char *f_pValue)
{
    int l_iRet = 0;
    char *l_pdest = NULL;
    unsigned int l_u32Num = 0;
    char *l_pLineStr = f_pLineStr;

    f_pNode[0] = 0;
    f_pValue[0] = 0;

    l_pdest = strstr(l_pLineStr, " = ");

    if (l_pdest != NULL)
    {
        // get node
        l_u32Num = l_pdest - f_pLineStr;
        strncpy(f_pNode, l_pLineStr, l_u32Num);
        f_pNode[l_u32Num] = 0;
        strcpy(f_pNode, remove_tow_sides(h, f_pNode));

        // get value
        strcpy(f_pValue, l_pdest + 1);
        strcpy(f_pValue, remove_tow_sides(h, f_pValue));

        l_iRet = 1;             // is valid

    }

    return l_iRet;
}

static void distill_tag(PARSE_TXT_HANDLE * h, char *f_pLine_str, char *f_pTag)
{
    char *pdest0 = NULL;
    char *pdest1 = NULL;

    pdest0 = strstr(f_pLine_str, "[");
    strcpy(f_pTag, pdest0 + 1);

    pdest1 = strstr(f_pTag, "]");
    *pdest1 = '\0';

    strcpy(f_pTag, remove_tow_sides(h, f_pTag));

}

static int parse_text(PARSE_TXT_HANDLE * h)
{
    DQ_HANDLE l_dqH = h->line_string_queue;
    char *l_pLingBuffer = h->pLine_str;
    char *l_pNode = h->pNode_str;
    char *l_pValue = h->pValue_str;
    char *l_pTag = h->pTag_str;
    S_LINE_STR_TYPE l_new_type = LINE_STR_TYPE_NULL;
    int l_iFindTag = 0;
    int l_iRet = 0;

    do
    {

        l_new_type = pop_line_string(l_dqH, l_pLingBuffer);

        if (l_new_type == LINE_STR_TYPE_TAG)
        {

            distill_tag(h, l_pLingBuffer, l_pTag);

            l_iRet = h->Tag_cbk(0, l_pTag, h->pTag_userdata);

            if (l_iRet == 0)
            {
                l_iFindTag = 1;
            }
            else if (l_iRet == 1)
            {
                l_iFindTag = 0; // try find next tag

                printf(" !!!!parse_config_info:: no need the tag = %s  \n", l_pTag);
            }
            else
            {
                printf(" break pares text in tag  \n");
                break;
            }

        }
        else if (l_new_type == LINE_STR_TYPE_CONTENT)
        {
            if (l_iFindTag && split_noed_value_string_line(h, l_pLingBuffer, l_pNode, l_pValue))
            {
                l_iRet = h->Node_cbk(0, l_pNode, l_pValue, h->pNode_userdata);

                if (l_iRet == 0)
                {
                    l_iFindTag = 1;
                }
                else if (l_iRet == 1)
                {
                    l_iFindTag = 0; // try find next tag

                    printf(" !!!!parse_config_info:: not the tag = %s  \n", l_pLingBuffer);
                }
                else
                {
                    printf(" break pares text in node  \n");
                    break;
                }

            }
        }
        else if (l_new_type == LINE_STR_TYPE_EROOR)
        {
            l_iFindTag = 0;

        }

    } while (l_new_type != LINE_STR_TYPE_NULL && h->run_flag);

    return 0;
}

/**
 * @brief         start parse txt
 * @param        h                                  parse txt handle
 * @param        Tag_cbk                    PT_TAG_CBK  function pointer ,when parse tag ,the parser will call it
 * @param        pTag_userdata            when call-back Tag_cbk, parser will return the value
 * @param        Node_cbk                    PT_NOED_CBK  function pointer ,when parse node ,the parser will call it
 * @param        pNode_userdata        when call-back Node_cbk, parser will return the value
 * @return         int type
 * @retval       0                                success
 * @retval       <0                            failled
 */
int pt_parse_start(PT_HANDLE h, PT_TAG_CBK Tag_cbk, void *pTag_userdata, PT_NOED_CBK Node_cbk, void *pNode_userdata)
{
    PARSE_TXT_HANDLE *l_hTxt = NULL;
    if (h == NULL || Tag_cbk == NULL || Node_cbk == NULL)
    {
        printf("pt_parse_start :: parameter is error !!! ");
        return -1;
    }

    l_hTxt = (PARSE_TXT_HANDLE *) h;

    l_hTxt->run_flag = 1;
    l_hTxt->Node_cbk = Node_cbk;
    l_hTxt->pNode_userdata = pNode_userdata;
    l_hTxt->Tag_cbk = Tag_cbk;
    l_hTxt->pTag_userdata = pTag_userdata;

    if (!data_queue_is_empty(l_hTxt->line_string_queue))
    {
        parse_text(l_hTxt);
    }

    return 0;
}

/**
 * @brief           destroy the parse txt handle
 * @param        h                                  parse txt handle
 * @return         int type
 * @retval       0                                success
 * @retval       <0                            failled
 */
int pt_parse_destroy(PT_HANDLE h)
{

    PARSE_TXT_HANDLE *l_hTxt = NULL;

    if (h == NULL)
    {
        printf("pt_parse_destroy :: parameter is error !!! ");
        return -1;
    }

    l_hTxt = (PARSE_TXT_HANDLE *) h;
    l_hTxt->run_flag = 0;

    clear_handle(l_hTxt);

    return 0;
}
