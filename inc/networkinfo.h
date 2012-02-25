/*
 * networkinfo.h
 *
 *  Created on: Aug 17, 2010
 *      Author: wangshuangxi
 */

#ifndef __NETWORKINFO_H__
#define __NETWORKINFO_H__

typedef enum
{
    ETH0 = 1,
    ETH1,
    ETH2,
    LO = 0X80
} S_ETH_NO;

typedef struct _NET_WORK_CARD_INFO_
{
    S_ETH_NO eth_no;
    int eth_open_flag;          // 0---> close ,1--->open
    char HWaddr[24];            // Mac addrress ,when eth_no = LO, the HWaddr is empty

} S_NET_WORK_CARD_INFO;

/**
 * @brief    get local networkcards info
 * @param    ppCards_array    [out]        double pointer    of S_NET_WORK_CARD_INFO type, the memory need allocated by being caller
 * @param    piCard_num        [out]        number of netcards
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_local_networkcards(S_NET_WORK_CARD_INFO ** ppCards_array, int *piCard_num);

/**
 * @brief    set  networkcards mac address
 * @param    eth_no            [in]            appointed eth
 * @param    pMac_address     [out]            mac address, the length of pMac_address buffer must be more than 18.
 And the format is hex_number5.hex_number4.hex_number3:hex_number2:hex_number1:hex_number0,
 e.g  00:0C:29:8B:2E:59
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_set_eth_mac(S_ETH_NO eth_no, char *pMac_address);

/**
 * @brief    get  networkcards ipv4 address
 * @param    eth_no            [in]            appointed eth
 * @param    ipv4_adrress     [out]            ipv4 address, the length of ipv4_adrress buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.110.233
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_eth_ipv4(S_ETH_NO eth_no, char *ipv4_adrress);
/**
 * @brief    set  networkcards ipv4 address
 * @param    eth_no            [in]            appointed eth
 * @param    ipv4_adrress     [in]            ipv4 address, the length of ipv4_adrress buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.110.233
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_set_eth_ipv4(S_ETH_NO eth_no, char *ipv4_adrress);

/**
 * @brief    get  child net mask
 * @param    eth_no                    [in]    appointed eth
 * @param    ipv4_child_netmask     [out]    ipv4 child net mask, the length of ipv4_child_netmask buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  255.255.255.0
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_eth_childnetmask(S_ETH_NO eth_no, char *ipv4_child_netmask);
/**
 * @brief    get  child net mask
 * @param    eth_no                    [in]    appointed eth
 * @param    ipv4_child_netmask     [in]    ipv4 child net mask, the length of ipv4_child_netmask buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  255.255.255.0
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_set_eth_childnetmask(S_ETH_NO eth_no, char *ipv4_child_netmask);

/**
 * @brief    get default gateway
 * @param    ipv4_gateway     [in]    ipv4 gateway, the length of ipv4_gateway buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.12.1
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_default_gateway(char *ipv4_gateway);
/**
 * @brief    set default gateway
 * @param    ipv4_gateway     [in]    ipv4 gateway, the length of ipv4_gateway buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.12.1
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_set_default_gateway(char *ipv4_gateway);

/**
 * @brief    open appointed eth
 * @param    eth_no                [in]    appointed eth
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_open_eth(S_ETH_NO eth_no);
/**
 * @brief    close appointed eth
 * @param    eth_no                [in]    appointed eth
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_close_eth(S_ETH_NO eth_no);

#endif /* NETWORKINFO_H_ */
