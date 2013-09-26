/*
 * parsetext.h
 *
 *  Created on: Aug 12, 2010
 *      Author: pete
 */

#ifndef __PARSETXT_H__
#define __PARSETXT_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *PT_HANDLE;

/**
 * @brief        parse tag call-back function prototype
 * @param    f_iParse_ret             parse ret
 *                                                         0   ---> success
 *                                                         <0  ----> error
 * @param    f_pTag                        tag txt info
 * @param    f_pTag_userdata        user data
 * @return      int type
 * @retval    0                                   continue  parse
 * @retval    1                                   continue to next tag
 * @retval   <0                               break  all parse
 */
typedef int (*PT_TAG_CBK) (int f_iParse_ret, char *f_pTag, void *f_pTag_userdata);

/**
 * @brief     parse    node call-back function prototype
 * @param    f_iParse_ret             parse ret
 *                                                         0   ---> success
 *                                                         <0  ----> error
 * @param    f_pNode                    node txt info
 * @param   f_pValue                    value of node
 * @param    f_pNode_userdata        user data
 * @return      int     type
 * @retval    0                                   continue  parse
 * @retval    1                                   continue to next tag
 * @retval   <0                               break  all parse
 */
typedef int (*PT_NOED_CBK) (int f_iParse_ret, char *f_pNode, char *f_pValue, void *f_pNode_userdata);

/**
 * @brief        create parse txt instance
 * @param    iTxt_format              text input format
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
PT_HANDLE pt_parse_create(int iTxt_format, char *f_pTxt_info);

/**
 * @brief         start parse txt
 * @param        h                                  parse text handle
 * @param        Tag_cbk                    PT_TAG_CBK  function pointer ,when parse tag ,the parser will call it
 * @param        pTag_userdata            when call-back Tag_cbk, parser will return the value
 * @param        Node_cbk                    PT_NOED_CBK  function pointer ,when parse node ,the parser will call it
 * @param        pNode_userdata        when call-back Node_cbk, parser will return the value
 * @return         int type
 * @retval       0                                success
 * @retval       <0                            failled
 */
int pt_parse_start(PT_HANDLE h, PT_TAG_CBK Tag_cbk, void *pTag_userdata, PT_NOED_CBK Node_cbk,
        void *pNode_userdata);

/**
 * @brief           destroy the parse txt handle
 * @param        h                                  parse text handle
 * @return         int type
 * @retval       0                                success
 * @retval       <0                            failled
 */
int pt_parse_destroy(PT_HANDLE h);

#ifdef __cplusplus
}                               /* extern "C" */
#endif

#endif /* __PARSETXT_H__ */
