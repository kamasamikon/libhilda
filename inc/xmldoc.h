/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __XMLDOC_H__
#define __XMLDOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ktypes.h>
#include <sdlist.h>

#include "expat.h"

typedef struct _KXmlNode {
	K_dlist_entry entry;
	K_dlist_entry subHdr;	/* KXmlNode */
	K_dlist_entry attrHdr;	/* KXmlAttr */
	struct _KXmlNode *parentNode;
	kchar *name;		/* Genre */
	kchar *text;		/* dddjnnd */
} KXmlNode;

typedef struct _KXmlAttr {
	K_dlist_entry entry;
	KXmlNode *parentNode;
	kchar *name;		/* xml:lang */
	kchar *value;		/* eng */
} KXmlAttr;

typedef struct _KXmlDoc {
	KXmlNode *cur_node;	/* NULL : root */
	KXmlNode *root;

	XML_Parser parser;
	kint parsedOffset;
} KXmlDoc;

#define setVal(a, v) \
do { \
	kmem_free(a); \
	if (v) { \
		a = kstr_dup(v); \
	} \
} while (0)

#define appendVal(os, ns, maxlen) \
do { \
	kint osl, nsl; \
	kchar *nbuf; \
	if (os) { \
		osl = strlen(os); \
		nsl = strlen(ns); \
		if (maxlen < nsl) nsl = maxlen; \
		nbuf = kmem_alloc(osl + nsl + 1); \
		memcpy(nbuf, os, osl); \
		memcpy(nbuf + osl, ns, nsl); \
		nbuf[osl + nsl] = 0; \
		kmem_free(os); \
	} else { \
		nsl = strlen(ns); \
		if (maxlen < nsl) nsl = maxlen; \
		nbuf = kmem_alloc(nsl + 1); \
		memcpy(nbuf, ns, nsl + 1); \
		nbuf[nsl] = 0; \
	} \
	os = nbuf; \
} while (0)

#define getVal(a, v) (a)->(v)

KXmlDoc *xmldoc_new(KXmlDoc *doc);
kint xmldoc_del(KXmlDoc *doc);
kvoid xmldoc_parse(KXmlDoc *doc, const kchar *buffer, kint buflen);
kint xmldoc_save(KXmlDoc *doc, const kchar *path);
kint xmldoc_getNodeCnt(KXmlDoc *doc, const kchar *path);
KXmlNode *xmldoc_getNode(KXmlDoc *doc, const kchar *path, kint index);
KXmlNode *xmldoc_gotoNode(KXmlDoc *doc, const kchar *path, kint index);
KXmlNode *xmldoc_addNode(KXmlDoc *doc, KXmlNode *node, KXmlNode *parent);
KXmlNode *xmlnode_new(KXmlNode *node, const kchar *name, const kchar *text);
kint xmlnode_set_name(KXmlNode *node, const kchar *name);
kint xmlnode_set_text(KXmlNode *node, const kchar *text);
kint xmlnode_set_attr_value(KXmlNode *node, const kchar *name, const kchar *value);
kchar *xmlnode_get_attr_value(KXmlNode *node, const kchar *name);
kint xmlnode_del(KXmlNode *node);
kint xmlnode_detach(KXmlNode *node);
KXmlNode *xmlnode_next(KXmlNode *node);
KXmlNode *xmlnode_next_same(KXmlNode *node);
KXmlNode *xmlnode_prev(KXmlNode *node);
KXmlNode *xmlnode_prev_same(KXmlNode *node);
kint xmlnode_addattr(KXmlNode *node, KXmlAttr *attr);
KXmlAttr *xmlnode_getattr(KXmlNode *node, const kchar *attrname);
KXmlAttr *xmlattr_new(KXmlAttr *attr, const kchar *name, const kchar *value);
kint xmlattr_del(KXmlAttr *attr);
kint xmlattr_detach(KXmlAttr *attr);
kint xmlattr_set_name(KXmlAttr *attr, const kchar *name);
kint xmlattr_set_value(KXmlAttr *attr, const kchar *value);

#ifdef __cplusplus
}
#endif
#endif /* __XMLDOC_H__ */
