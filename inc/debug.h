/*********************************************************************************************************
 * Copyright (C), 1998-2010, Beijing Novel-SuperTV Digital TV Technology Co., Ltd.
 * File Name     : debug.h
 * Author        : liushuyong
 * Created       : 2011-4-26
 * Description   :
 *
 * History       :
 *                  1.	Date        	: 2011-4-26
 *   				  	Author      	: liushuyong
 *                   	Modification	: Create
 *********************************************************************************************************/
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef __cplusplus
extern "C"
{
#endif

//Author:A
//Moudle:M
//example  A:shuyong
//		   File		:cloud_server_if
//		   Line		:888
//		   Func		:opt_set_TargetAddrDescriptor
//		   MSG_LEVEL:[TRACE]
//		   ....

#define TRACE_LEVEL

//<跟踪级别
#if defined(TRACE_LEVEL)
	#define TRACE(AuthorOrMoudle, format, args...)												\
		printf("\nContact  :%s\n"					\
			   "File     :%s\n"		\
			   "Line     :%d\n"			\
			   "Func     :%s\n"		\
			   "MSG_LEVEL:[TRACE]\n"	\
				"MSG     :"format"\n",			\
				AuthorOrMoudle,			\
				__FILE__,				\
				__LINE__,				\
				__func__,				\
				##args );
#else
	#define TRACE(AuthorOrMoudle, format, args...)

#endif

//<调试级别
#if defined(TRACE_LEVEL) || defined(DEBUG_LEVEL)
	#define DEBUG(AuthorOrMoudle, format, args...)												\
		printf("\nContact  :%s\n"					\
			   "File     :%s\n"		\
			   "Line     :%d\n"			\
			   "Func     :%s\n"		\
			   "MSG_LEVEL:[DEBUG]\n"	\
			   "MSG      :"format"\n",			\
				AuthorOrMoudle,			\
				__FILE__,				\
				__LINE__,				\
				__func__,				\
				##args );
#else
	#define DEBUG(AuthorOrMoudle, format, args...) 												\
		;
#endif


//<信息级别
#if defined(TRACE_LEVEL) || defined(DEBUG_LEVEL) || defined(INFO_LEVEL)
	#define INFO(AuthorOrMoudle, format, args...)												\
		printf("\nContact  :%s\n"					\
			   "File     :%s\n"		\
			   "Line     :%d\n"			\
			   "Func     :%s\n"		\
			   "MSG_LEVEL:[INFO]\n"	\
			   "MSG      :"format"\n",			\
				AuthorOrMoudle,			\
				__FILE__,				\
				__LINE__,				\
				__func__,				\
				##args );
#else
	#define INFO(AuthorOrMoudle, format, args...) 														  	\
		;
#endif

//<警告级别
#if defined(TRACE_LEVEL) || defined(DEBUG_LEVEL) || defined(INFO_LEVEL) || defined(WARNING_LEVEL)
	#define WARNING(AuthorOrMoudle, format, args...)												\
		printf( "\nContact  :%s\n"			\
				"File     :%s\n"			\
				"Line     :%d\n"			\
				"Func     :%s\n"			\
				"MSG_LEVEL:[WARNING]\n"	\
				"MSG      :"format"\n",			\
				AuthorOrMoudle,			\
				__FILE__,				\
				__LINE__,				\
				__func__,				\
				##args );
#else
	#define WARNING(AuthorOrMoudle, format, args...) 												\
		;
#endif

//<错误级别（发布版本的默认输出信息级别）system error
#if defined(TRACE_LEVEL) || defined(DEBUG_LEVEL) || defined(INFO_LEVEL) || defined(WARNING_LEVEL) || defined(ERROR_LEVEL)
	#define ERROR_SYS(AuthorOrMoudle)														\
		printf( "\nContact   :%s\n"			\
				"File     :%s\n"			\
				"Line     :%d\n"			\
				"Func     :%s\n"			\
				"MSG_LEVEL:[SYS_ERROR]\n"	\
				"MSG      :""%s\n",			\
				AuthorOrMoudle,			\
				__FILE__,				\
				__LINE__,				\
				__func__,				\
				strerror(errno));
#else
	#define ERROR_SYS() 														\
		;
#endif

//<错误级别（发布版本的默认输出信息级别）	project error
#if defined(TRACE_LEVEL) || defined(DEBUG_LEVEL) || defined(INFO_LEVEL) || defined(WARNING_LEVEL) || defined(ERROR_LEVEL)
	#define ERROR_PRJ(AuthorOrMoudle, format, args...)											\
		printf( "\nContact  :%s\n"			\
				"File     :%s\n"			\
				"Line     :%d\n"			\
				"Func     :%s\n"			\
				"MSG_LEVEL:[PRJ_ERROR]\n"	\
				"MSG      :"format"\n",			\
				AuthorOrMoudle,			\
				__FILE__,				\
				__LINE__,				\
				__func__,				\
				##args );
#else
	#define ERROR_PRJ(AuthorOrMoudle, format, args...) 										\
		;
#endif

#ifdef __cplusplus
}
#endif

#endif
