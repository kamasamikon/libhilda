/*
 * networkinfo.c
 *
 *  Created on: Aug 17, 2010
 *      Author: wangshuangxi
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <bits/errno.h>
#include <ctype.h>
#include <asm-generic/errno-base.h>

#include "networkinfo.h"

int CheckNetLink(const char *routeName)
{
    int fd, intrface;
    struct ifreq buf[16];
    struct ifconf ifc;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t) buf;
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
        {
            intrface = ifc.ifc_len / sizeof(struct ifreq);
            while (intrface-- > 0)
            {
                if (strcmp(buf[intrface].ifr_name, routeName) == 0)
                {
                    ioctl(fd, SIOCGIFFLAGS, (char *)&buf[intrface]);
                    if ((buf[intrface].ifr_flags & IFF_UP) && (buf[intrface].ifr_flags & IFF_RUNNING))
                    {
                        printf("\n%s is UP & RUNNING\n", routeName);
                        return 0;
                    }
                    else
                    {
                        printf("\n%s is DOWN\n", routeName);
                        return -1;
                    }
                }
            }
            printf("\n%s is not exist\n", routeName);
            return -1;
        }
    }
    return -1;
}

/**
 * @brief    get local networkcards info
 * @param    ppCards_array    [out]        double pointer    of S_NET_WORK_CARD_INFO type, the memory need allocated by being caller
 * @param    piCard_num        [out]        number of netcards
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_local_networkcards(S_NET_WORK_CARD_INFO ** ppCards_array, int *piCard_num)
{
    int numEth0 = CheckNetLink("eth0");
    int numEth1 = CheckNetLink("eth1");
    *piCard_num = numEth0 + numEth1 + 2;
    printf("Without realizing\n");
    return -1;
}

int cnw3602_set_mac_addr(char *eth_no, char *macaddr)
{
    char tmp[40];
    sprintf(tmp, "ifconfig %s hw ether %s", eth_no, macaddr);
    if (system(tmp) == 0)
    {
        printf("set %s mac ok\n", eth_no);
    }
    else
    {
        printf("set %s mac error\n", eth_no);
        return -1;
    }
    return 0;
}

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
int ntw_set_eth_mac(S_ETH_NO eth_no, char *pMac_address)
{
    int l_ret = 0;
    switch (eth_no)
    {
    case ETH0:
        {
            l_ret = cnw3602_set_mac_addr("eth0", pMac_address);
            printf("set eth0 mac\n");
            break;
        }
    case ETH1:
        {
            l_ret = cnw3602_set_mac_addr("eth1", pMac_address);
            printf("set eth1 mac\n");
            break;
        }
    case LO:
        {
            printf("LO not have mac addr\n");
            return -1;
            break;
        }
    default:
        break;
    }

    return l_ret;
}

int cnw3602_get_local_ip(char *eth, char *get_ipaddr)
{
    int fd = 0;
    int ret = -1;
    struct sockaddr_in *sin;
    struct ifreq ifr_ip;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }
    memset(&ifr_ip, 0, sizeof(ifr_ip));
    strncpy(ifr_ip.ifr_ifrn.ifrn_name, eth, (sizeof(ifr_ip.ifr_ifrn.ifrn_name) - 1));
    ret = ioctl(fd, SIOCGIFADDR, &ifr_ip);
    close(fd);
    if (ret < 0)
        return -1;
    sin = (struct sockaddr_in *)&ifr_ip.ifr_ifru.ifru_addr;
    strcpy(get_ipaddr, inet_ntoa(sin->sin_addr));
    printf("get local ip:%s \n", get_ipaddr);
    return 0;
}

/**
 * @brief    get  networkcards ipv4 address
 * @param    eth_no            [in]            appointed eth
 * @param    ipv4_adrress     [out]            ipv4 address, the length of ipv4_adrress buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.110.233
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_eth_ipv4(S_ETH_NO eth_no, char *ipv4_adrress)
{
    int l_ret = 0;
    switch (eth_no)
    {
    case ETH0:
        {
            l_ret = cnw3602_get_local_ip("eth0", ipv4_adrress);
            break;
        }
    case ETH1:
        {
            l_ret = cnw3602_get_local_ip("eth1", ipv4_adrress);
            break;
        }
    case LO:
        {
            printf("Without realizing\n");
            return -1;
            break;
        }
    default:
        break;
    }

    return l_ret;
}

int cnw3602_set_local_ip(char *eth, char *set_ip)
{
    int fd = 0;
    int ret = -1;
    struct sockaddr_in *sin;
    struct ifreq ifr_ip;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }
    memset(&ifr_ip, 0, sizeof(ifr_ip));
    strncpy(ifr_ip.ifr_ifrn.ifrn_name, eth, (sizeof(ifr_ip.ifr_ifrn.ifrn_name) - 1));
    sin = (struct sockaddr_in *)&ifr_ip.ifr_ifru.ifru_addr;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr(set_ip);
    ret = ioctl(fd, SIOCSIFADDR, &ifr_ip);
    close(fd);
    if (ret < 0)
        return -1;
    strcpy(set_ip, inet_ntoa(sin->sin_addr));
    printf("set local ip:%s \n", set_ip);
    return 0;
}

/**
 * @brief    set  networkcards ipv4 address
 * @param    eth_no            [in]            appointed eth
 * @param    ipv4_adrress     [in]            ipv4 address, the length of ipv4_adrress buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.110.233
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_set_eth_ipv4(S_ETH_NO eth_no, char *ipv4_adrress)
{
    int l_ret = 0;
    switch (eth_no)
    {
    case ETH0:
        {
            l_ret = cnw3602_set_local_ip("eth0", ipv4_adrress);
            break;
        }
    case ETH1:
        {
            l_ret = cnw3602_set_local_ip("eth1", ipv4_adrress);
            break;
        }
    case LO:
        {
            printf("Without realizing\n");
            return -1;
            break;
        }
    default:
        break;
    }

    return l_ret;
}

int cnw3602_get_local_netmask(char *eth, char *get_netmask)
{
    int net_mask_fd = 0;
    int ret = -1;
    struct sockaddr_in *net_mask;
    struct ifreq ifr_mask;
    if ((net_mask_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }
    memset(&ifr_mask, 0, sizeof(ifr_mask));
    strncpy(ifr_mask.ifr_ifrn.ifrn_name, eth, (sizeof(ifr_mask.ifr_ifrn.ifrn_name) - 1));
    ret = ioctl(net_mask_fd, SIOCGIFNETMASK, &ifr_mask);
    close(net_mask_fd);
    if (ret < 0)
        return -1;
    net_mask = (struct sockaddr_in *)&ifr_mask.ifr_ifru.ifru_netmask;
    strcpy(get_netmask, inet_ntoa(net_mask->sin_addr));
    printf("get local netmask:%s \n", get_netmask);
    return 0;
}

/**
 * @brief    get  child net mask
 * @param    eth_no                    [in]    appointed eth
 * @param    ipv4_child_netmask     [out]    ipv4 child net mask, the length of ipv4_child_netmask buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  255.255.255.0
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_eth_childnetmask(S_ETH_NO eth_no, char *ipv4_child_netmask)
{
    int l_ret = 0;
    switch (eth_no)
    {
    case ETH0:
        {
            l_ret = cnw3602_get_local_netmask("eth0", ipv4_child_netmask);
            break;
        }
    case ETH1:
        {
            l_ret = cnw3602_get_local_netmask("eth1", ipv4_child_netmask);
            break;
        }
    case LO:
        {
            printf("Without realizing\n");
            return -1;
            break;
        }
    default:
        break;
    }

    return l_ret;
}

int cnw3602_set_local_netmask(char *eth, char *netmask)
{
    int net_mask_fd = 0;
    int ret = -1;
    struct sockaddr_in *sin_net_mask;
    struct ifreq ifr_mask;

    if ((net_mask_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }
    memset(&ifr_mask, 0, sizeof(ifr_mask));
    strncpy(ifr_mask.ifr_ifrn.ifrn_name, eth, (sizeof(ifr_mask.ifr_ifrn.ifrn_name) - 1));
    sin_net_mask = (struct sockaddr_in *)&ifr_mask.ifr_ifru.ifru_netmask;
    sin_net_mask->sin_family = AF_INET;
    inet_pton(AF_INET, netmask, &sin_net_mask->sin_addr);
    ret = ioctl(net_mask_fd, SIOCSIFNETMASK, &ifr_mask);
    if (ret < 0)
        return -1;
    close(net_mask_fd);
    return 0;
}

/**
 * @brief    get  child net mask
 * @param    eth_no                    [in]    appointed eth
 * @param    ipv4_child_netmask     [in]    ipv4 child net mask, the length of ipv4_child_netmask buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  255.255.255.0
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_set_eth_childnetmask(S_ETH_NO eth_no, char *ipv4_child_netmask)
{
    int l_ret = 0;
    switch (eth_no)
    {
    case ETH0:
        {
            l_ret = cnw3602_set_local_netmask("eth0", ipv4_child_netmask);
            break;
        }
    case ETH1:
        {
            l_ret = cnw3602_set_local_netmask("eth1", ipv4_child_netmask);
            break;
        }
    case LO:
        {
            printf(" LO set Without realizing\n");
            return -1;
            break;
        }
    default:
        break;
    }

    return l_ret;
}

int cnw3602_get_defgateway(char *get_gateway)
{
    FILE *fp;
    char buf[512];
    char cmd[128];
    char *tmp;

    strcpy(cmd, "route");
    fp = popen(cmd, "r");
    if (NULL == fp)
    {
        perror("popen error");
        return -1;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        tmp = buf;
        while (*tmp && isspace(*tmp))
            ++tmp;
        if (strncmp(tmp, "default", strlen("default")) == 0)
            break;
    }
    sscanf(buf, "%*s%s", get_gateway);
    printf("default gateway:%s\n", get_gateway);
    pclose(fp);

    return 0;
}

/**
 * @brief    get default gateway
 * @param    ipv4_gateway     [in]    ipv4 gateway, the length of ipv4_gateway buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.12.1
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_get_default_gateway(char *ipv4_gateway)
{
    int l_ret = 0;
    l_ret = cnw3602_get_defgateway(ipv4_gateway);
    return l_ret;
}

int cnw3602_set_defgateway(const char *defGateway)
{
    int sockfd;
    struct rtentry rm;
    int err;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        printf("socket is -1\n");
        return -1;
    }

    memset(&rm, 0, sizeof(rm));
    ((struct sockaddr_in *)&rm.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rm.rt_dst)->sin_addr.s_addr = 0;

    ((struct sockaddr_in *)&rm.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rm.rt_genmask)->sin_addr.s_addr = 0;

    rm.rt_flags = RTF_UP;

    if ((err = ioctl(sockfd, SIOCDELRT, &rm)) < 0)
    {
        printf("SIOCDELRT failed , ret->%d, no default default gateway\n", err);
        // return -1;
    }

    memset(&rm, 0, sizeof(rm));
    ((struct sockaddr_in *)&rm.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rm.rt_dst)->sin_addr.s_addr = 0;

    ((struct sockaddr_in *)&rm.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rm.rt_genmask)->sin_addr.s_addr = 0;

    ((struct sockaddr_in *)&rm.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rm.rt_gateway)->sin_addr.s_addr = inet_addr(defGateway);

    rm.rt_flags = RTF_UP | RTF_GATEWAY;

    if ((err = ioctl(sockfd, SIOCADDRT, &rm)) < 0)
    {
        printf("SIOCADDRT failed , ret->%d\n", err);
        return -1;
    }
    return 0;
}

/**
 * @brief    set default gateway
 * @param    ipv4_gateway     [in]    ipv4 gateway, the length of ipv4_gateway buffer must be more than 16.
 And the format is decimal_number3.decimal_number2.decimal_number1.decimal_number0, e.g  192.168.12.1
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_set_default_gateway(char *ipv4_gateway)
{
    int l_ret = 0;
    l_ret = cnw3602_set_defgateway(ipv4_gateway);
    return l_ret;
}

int cnw3602_set_network_device_up(char *eth)
{
    char tmp[20];
    sprintf(tmp, "ifconfig %s up", eth);
    if (system(tmp) == 0)
    {
        printf("ifconfig %s up ok\n", eth);
    }
    else
    {
        printf("ifconfig %s up error\n", eth);
        return -1;
    }
    return 0;
}

/**
 * @brief    open appointed eth
 * @param    eth_no                [in]    appointed eth
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_open_eth(S_ETH_NO eth_no)
{
    int l_ret = 0;
    switch (eth_no)
    {
    case ETH0:
        {
            l_ret = cnw3602_set_network_device_up("eth0");
            break;
        }
    case ETH1:
        {
            l_ret = cnw3602_set_network_device_up("eth1");
            break;
        }
    case LO:
        {
            printf(" LO is open\n");
            break;
        }
    default:
        break;
    }

    return l_ret;
}

int cnw3602_set_network_device_down(char *eth)
{
    char tmp[20];
    sprintf(tmp, "ifconfig %s down", eth);
    if (system(tmp) == 0)
    {
        printf("ifconfig %s down ok\n", eth);
    }
    else
    {
        printf("ifconfig %s down error\n", eth);
        return -1;
    }
    return 0;
}

/**
 * @brief    close appointed eth
 * @param    eth_no                [in]    appointed eth
 * @return   return int type
 * @retval   success             0
 * @retval   failed             <0
 */
int ntw_close_eth(S_ETH_NO eth_no)
{
    int l_ret = 0;
    switch (eth_no)
    {
    case ETH0:
        {
            l_ret = cnw3602_set_network_device_down("eth0");
            break;
        }
    case ETH1:
        {
            l_ret = cnw3602_set_network_device_down("eth1");
            break;
        }
    case LO:
        {
            printf(" LO set Without realizing\n");
            break;
        }
    default:
        break;
    }

    return l_ret;
}
