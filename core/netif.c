/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include <asm/types.h>

#include <linux/sockios.h>
#include <linux/ethtool.h>

#include <hilda/netif.h>

#include <hilda/sysdeps.h>

#ifdef CFG_NETIF_TEST
int main(void)
{
	int err;

	int isup;
	char ipaddr[256], netmsc[256], hwaddr[256];
	err = netinf_get("eth1", ipaddr, &isup, netmsc, hwaddr);
	if (!err) {
		printf("ipaddr: %s\n", ipaddr);
		printf("isup: %d\n", isup);
		printf("netmsc: %s\n", netmsc);
		printf("hwaddr: %s\n", hwaddr);

		err = netinf_set("eth1", ipaddr, &isup, netmsc, hwaddr);
	}
	return 0;
}
#endif /* CFG_NETIF_TEST */

static int get_ipaddr(int s, struct ifreq *ifr, char ipaddr[])
{
	int err = 0;

	if (ioctl(s, SIOCGIFADDR, (char *)ifr) >= 0)
		sprintf(ipaddr, "%s", inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr));
	else
		err = -1;

	return err;
}

static int get_isup(int s, struct ifreq *ifr, int *isup)
{
	int err = 0;

	if (ioctl(s, SIOCGIFFLAGS, (char *)ifr) >= 0)
		*isup = (ifr->ifr_flags & IFF_UP) ? 1 : 0;
	else
		err = -1;

	return err;
}

static int get_netmask(int s, struct ifreq *ifr, char netmask[])
{
	int err = 0;
	struct sockaddr_in *sin;

	if (ioctl(s, SIOCGIFNETMASK, ifr) >= 0) {
		sin = (struct sockaddr_in *)&ifr->ifr_netmask;
		inet_ntop(AF_INET, &(sin->sin_addr), netmask, 20);
	} else
		err = -1;

	return err;
}

static int get_hwaddr(int s, struct ifreq *ifr, char hwaddr[])
{
	int err = 0;

	if (ioctl(s, SIOCGIFHWADDR, (char *)ifr) >= 0) {
		unsigned char *hw = (unsigned char *)ifr->ifr_hwaddr.sa_data;
		sprintf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
				hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
	} else
		err = -1;

	return err;
}

int netinf_get(const char *dev, char ipaddr[], int *isup,
		char netmask[], char hwaddr[])
{
	int s, err = 0;
	struct ifreq ifr;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, dev);

	if (!err && ipaddr)
		err = get_ipaddr(s, &ifr, ipaddr);
	if (!err && isup)
		err = get_isup(s, &ifr, isup);
	if (!err && netmask)
		err = get_netmask(s, &ifr, netmask);
	if (!err && hwaddr)
		err = get_hwaddr(s, &ifr, hwaddr);

	close(s);
	return err;
}

static kinline void init_sockaddr_in(struct sockaddr_in *sin, const char *addr)
{
	sin->sin_family = AF_INET;
	sin->sin_port = 0;
	sin->sin_addr.s_addr = inet_addr(addr);
}

int netinf_set(const char *dev, char ipaddr[], int *isup,
		char netmask[], char hwaddr[])
{
	int s, err = 0;
	struct ifreq ifr;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, dev);

	if (!err && ipaddr) {
		init_sockaddr_in((struct sockaddr_in *)&ifr.ifr_addr, ipaddr);
		if (ioctl(s, SIOCSIFADDR, &ifr) < 0)
			err = -1;
	}

	if (!err && isup) {
		err = -1;
		if (ioctl(s, SIOCGIFFLAGS, ifr) < 0) {
			if (*isup)
				ifr.ifr_flags |= IFF_UP;
			else
				ifr.ifr_flags &= ~IFF_UP;

			if (ioctl(s, SIOCSIFFLAGS, &ifr) >= 0)
				err = 0;
		}
	}

	if (!err && netmask) {
		init_sockaddr_in((struct sockaddr_in *) &ifr.ifr_netmask,
				netmask);
		if (ioctl(s, SIOCSIFNETMASK, &ifr) < 0) {
			perror("SIOCSIFNETMASK");
			err = -1;
		}
	}

	if (!err && hwaddr) {
		unsigned char hw[24];
		sscanf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
				(unsigned int*)&hw[0], (unsigned int*)&hw[1],
				(unsigned int*)&hw[2], (unsigned int*)&hw[3],
				(unsigned int*)&hw[4], (unsigned int*)&hw[5]);

		ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
		memcpy(ifr.ifr_hwaddr.sa_data, hw, 6);
		if (ioctl(s, SIOCSIFHWADDR, &ifr) < 0)
			err = -1;
	}

	close(s);
	if (err)
		printf("\n\n>>>>>>>>>>>>>>>> netinf_set failed: errno:%d, dev:%s, ipaddr:%s, isup:%d, netmask:%s, hwaddr:%s\n\n",
				errno, dev, ipaddr ? : "(null)", isup ? *isup : -1, netmask ? : "(null)", hwaddr ? : "(null)");
	return err;
}

int netinf_get_gw(const char *dev, char gw[])
{
	FILE *fp;
	char buf[256];

	if (!dev)
		return -1;

	sprintf(buf, "route -n | grep %s | grep \"^0.0.0.0\" | awk '{print $2}' | tr -d '\n'", dev);
	fp = popen(buf, "r");
	if (!fp)
		return -1;

	memset(buf, 0, sizeof(buf));
	(void)fread(buf, sizeof(char), sizeof(buf), fp);
	pclose(fp);

	/* strlen("0.0.0.0") == 7 */
	if (strlen(buf) < 7)
		return -1;
	strcpy(gw, buf);

	return 0;
}

int netinf_set_gw(const char *dev, const char *gw)
{
	char cmd[256];
	int err = 0;

	if (!dev || !gw)
		return -1;

	while (!err)
		err = system("route del default");

	sprintf(cmd, "route add default gw %s dev %s", gw, dev);

	err = system(cmd);

	if (err)
		printf("\n\n>>>>>>>>>>>>>>>> netinf_set_gw failed: errno:%d, dev:%s, gw:%s\n\n",
				errno, dev, gw ? : "(null)");

	if (err)
		return -1;

	return 0;
}

static int dot_count(const char *addr)
{
	int dot = 0;
	const char *p;

	for (p = addr; *p; p++)
		if (*p == '.')
			dot++;

	return dot;
}

static int netinf_chk_ipaddr(const char *ipaddr)
{
	in_addr_t addr;

	if (dot_count(ipaddr) != 3)
		return -1;

	addr = inet_network(ipaddr);
	if (addr == -1)
		return -1;

	return 0;
}

static int netinf_chk_netmask(const char *netmask)
{
	in_addr_t addr;
	int i;

	if (dot_count(netmask) != 3)
		return -1;

	/* XXX: 255.255.255.255 will return -1 */
	addr = inet_network(netmask);
	if (addr == -1)
		return -1;

	for (i = 31; i; i--)
		if (!(addr & (1 << i)))
			break;
	/* No 1 after zero found */
	for (; i; i--)
		if (addr & (1 << i))
			break;

	return (i == 0) ? 0 : -1;
}

static int netinf_chk_ipaddr_netmask(const char *ipaddr, const char *netmask)
{
	in_addr_t addr_addr, addr_mask;
	in_addr_t addr_sub;

	addr_addr = inet_network(ipaddr);
	addr_mask = inet_network(netmask);

	/* XXX: sub net must not be zero */
	addr_sub = addr_addr & (~addr_mask);

	if (addr_sub == 0)
		return -1;

	if ((addr_sub & (~addr_mask)) == (~addr_mask))
		return -1;

	return 0;
}

static int netinf_chk_gw(const char *gw)
{
	return netinf_chk_ipaddr(gw);
}

static int netinf_chk_gw_netmask(const char *gw, const char *netmask)
{
	return netinf_chk_ipaddr_netmask(gw, netmask);
}
/* ipaddr: 0x0001, netmask: 0x0010, gw: 0x0100 */
static int netinf_chk_ip_gw(const char *ip, const char *gw, const char *netmask)
{
	in_addr_t addr_addr, addr_gw, addr_mask;
	in_addr_t net_ip, net_gw;

	addr_addr = inet_network(ip);
	addr_gw = inet_network(gw);
	addr_mask = inet_network(netmask);

	/* XXX: get net addr  */
	net_ip = addr_addr & addr_mask;
	net_gw = addr_gw & addr_mask;

	if( net_ip == net_gw )
		return 0;
	else
		return -1;
}
/* ipaddr: 0x0001, netmask: 0x0010, gw: 0x0100 */
int netinf_chk(const char *ipaddr, const char *netmask, const char *gw)
{
	if (ipaddr && netmask && gw)
		if(netinf_chk_ip_gw(ipaddr, gw, netmask))
			return 0x0111;

	if (ipaddr && netmask)
		if (netinf_chk_ipaddr_netmask(ipaddr, netmask))
			return 0x0011;

	if (gw && netmask)
		if (netinf_chk_gw_netmask(gw, netmask))
			return 0x0110;

	if (ipaddr)
		if (netinf_chk_ipaddr(ipaddr))
			return 0x0001;

	if (netmask)
		if (netinf_chk_netmask(netmask))
			return 0x0010;

	if (gw)
		if (netinf_chk_gw(gw))
			return 0x0100;

	return 0;
}

int netinf_islink(const char *dev)
{
	int s;
	struct ifreq ifr;
	struct ethtool_value edata;

	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (char *) &edata;
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
		return -1;
	if (ioctl(s, SIOCETHTOOL, &ifr) == -1) {
		close(s);
		return -1;
	}
	close(s);
	return edata.data;
}

