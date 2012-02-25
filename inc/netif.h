/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NET_IF_H__
#define __NET_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

int netinf_get(const char *dev, char ipaddr[], int *isup,
		char netmask[], char hwaddr[]);
int netinf_set(const char *dev, char ipaddr[], int *isup,
		char netmask[], char hwaddr[]);

int netinf_get_gw(const char *dev, char gw[]);
int netinf_set_gw(const char *dev, const char *gw);

int netinf_chk(const char *ipaddr, const char *netmask, const char *gw);

int netinf_islink(const char *dev);

#ifdef __cplusplus
}
#endif
#endif /* __NET_IF_H__ */

