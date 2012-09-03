/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <kmem.h>
#include <kstr.h>
#include <xtcool.h>

#include <xmldoc.h>

/**
 * @name expatCallbacks
 * @{
 */

/**
 * @brief Start a XML element
 *
 * @param userData KXmlDoc
 * @param name element name
 * @param atts element attributes
 */
static void onStartElement(void *userData, const XML_Char *name, const XML_Char **atts)
{
	kint i;
	KXmlDoc *doc = (KXmlDoc*)userData;
	KXmlNode *node = xmlnode_new(knil, (kchar*)name, knil);
	KXmlAttr *attr;

	xmldoc_addNode(doc, node, doc->cur_node);
	doc->cur_node = node;

	/* (i)th is attribution name, (i+1)th is attribution value */
	for (i = 0; atts[i]; i += 2) {
		attr = xmlattr_new(knil, (kchar*)atts[i], (kchar*)atts[i + 1]);
		xmlnode_addattr(node, attr);
	}
}

/**
 * @brief A XML element end
 *
 * @param userData KXmlDoc
 * @param name element name
 */
static void onEndElement(void *userData, const XML_Char *name)
{
	KXmlDoc *doc = (KXmlDoc*)userData;
	kassert(doc);

	xmldoc_gotoNode(doc, "..", 0);
}

#define ISSPACE(c) (((c) == 0x20) || ((c) == 0x0D) || ((c) == 0x0A) || ((c) == 0x09))

/**
 * @brief Found element data or value
 *
 * @param userData KXmlDoc
 * @param s element value as a string
 * @param len strlen(s)
 *
 * @warning This function will called for every line for every element, and DO NOT strip any white spaces
 */
static void onCharacterData(void *userData, const XML_Char *s, int len)
{
	kint osl, nsl;
	kchar *nbuf;
	KXmlNode *node = ((KXmlDoc*)userData)->cur_node;

	if ((node->text)) {
		osl = strlen((node->text));
		nsl = len;
		nbuf = kmem_alloc(osl + nsl + 1, char);
		memcpy(nbuf, (node->text), osl);
		memcpy(nbuf + osl, (s), nsl);
		nbuf[osl + nsl] = 0;
		kmem_free((node->text));
	} else {
		nsl = len;
		nbuf = kmem_alloc(nsl + 1, char);
		memcpy(nbuf, (s), nsl + 1);
		nbuf[nsl] = 0;
	}
	(node->text) = nbuf;
}

/** @} */

/**
 * @brief Construct a xml doc.
 *
 * @param doc if set, use memory by doc
 *
 * @return Pointer to KXmlDoc when success, knil for error
 */
KXmlDoc *xmldoc_new(KXmlDoc *doc)
{
	XML_Memory_Handling_Suite mhs;
	mhs.malloc_fcn = kmem_get;
	mhs.realloc_fcn = kmem_reget;
	mhs.free_fcn = kmem_rel;

	if (!doc)
		doc = (KXmlDoc*)kmem_alloz(1, KXmlDoc);

	if (doc) {
		doc->root = xmlnode_new(knil, "/", "root");
		doc->cur_node = doc->root;
		doc->parser = XML_ParserCreate_MM(knil, &mhs, knil);
		if (doc->parser == knil) {
			kerror(("create xml parser failed\n"));
			xmldoc_del(doc);
			doc = knil;
		}
	}
	return doc;
}

/**
 * @brief Destroy KXmlDoc
 *
 * @param doc KXmlDoc
 *
 * @return 0 for success, -1 for error
 */
kint xmldoc_del(KXmlDoc *doc)
{
	if (doc) {
		/* free the sub node */
		XML_ParserFree(doc->parser);
		doc->parser = knil;

		/* free all the sub node */
		if (doc->root) {
			xmlnode_del(doc->root);
			doc->root = knil;
		}

		kmem_free(doc);
		return 0;
	}
	return -1;
}

/**
 * @brief Parse the XML raw data into KXmlDoc tree
 *
 * @param doc KXmlDoc
 * @param buffer buffer be parsed.
 * @param buflen strlen(buffer).
 */
kvoid xmldoc_parse(KXmlDoc *doc, const kchar *buffer, kint buflen)
{
	XML_SetUserData(doc->parser, doc);
	XML_SetElementHandler(doc->parser, onStartElement, onEndElement);
	XML_SetCharacterDataHandler(doc->parser, onCharacterData);

	XML_Parse(doc->parser, buffer, buflen, 1);
	doc->parsedOffset = XML_GetCurrentByteIndex(doc->parser);

	/* reset current node to root */
	doc->cur_node = doc->root;
}

/**
 * @brief print XML node to file
 *
 * @param node KXmlNode
 * @param fp target file
 * @param level indent level
 */
static kint xmlnode_print(KXmlNode *node, kbean fp, kint *level)
{
	KXmlAttr *attr;
	KXmlNode *subnode;
	K_dlist_entry *entry, *hdr;
	kint i, subCnt = 0, textCnt = 0;

	kvfs_printf(fp, "\n");
	for (i = 0; i < *level; i++)
		kvfs_printf(fp, "\t");
	kvfs_printf(fp, "<%s", node->name);

	/* fill attributions */
	hdr = &node->attrHdr;
	entry = hdr->next;
	while (entry != hdr) {
		attr = FIELD_TO_STRUCTURE(entry, KXmlAttr, entry);
		entry = entry->next;

		kvfs_printf(fp, " %s=\"%s\"", attr->name, attr->value);
	}

	kvfs_printf(fp, ">");

	(*level)++;

	/* fill sub */
	hdr = &node->subHdr;
	entry = hdr->next;
	while (entry != hdr) {
		subnode = FIELD_TO_STRUCTURE(entry, KXmlNode, entry);
		entry = entry->next;

		xmlnode_print(subnode, fp, level);
		subCnt++;
	}

	(*level)--;

	if (node->text) {
		kvfs_printf(fp, "%s", node->text);
		textCnt++;
	}

	if (subCnt) {
		kvfs_printf(fp, "\n");
		for (i = 0; i < *level; i++)
			kvfs_printf(fp, "\t");
	}

	if (!(*level))
		kvfs_printf(fp, "</%s>", node->name);
	else
		kvfs_printf(fp, "</%s>", node->name);

	return 0;
}

/**
 * @brief save KXmlDoc as a XML file
 *
 * @param doc KXmlDoc
 * @param path target file name
 */
kint xmldoc_save(KXmlDoc *doc, const kchar *path)
{
	KXmlNode *node;
	KXmlNode *subnode;
	K_dlist_entry *entry, *hdr;
	kint level = 0;

	kbean fp = (kbean) kvfs_open((kchar*)path, "w+t", 0);
	if (!fp)
		return -1;

	node = xmldoc_getNode(doc, "/", 0);

	/* XXX default to be UTF-8 */
	kvfs_printf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

	/* fill sub */
	hdr = &node->subHdr;
	entry = hdr->next;
	while (entry != hdr) {
		subnode = FIELD_TO_STRUCTURE(entry, KXmlNode, entry);
		entry = entry->next;

		xmlnode_print(subnode, fp, &level);
	}

	kvfs_close(fp);
	return 0;
}

kint xmldoc_getNodeCnt(KXmlDoc *doc, const kchar *path)
{
	K_dlist_entry *entry;
	KXmlNode *node;
	kint curIndex = 0;
	if (doc && path) {
		KXmlNode *curnode = doc->cur_node;

		entry = curnode->subHdr.next;
		while (entry != &curnode->subHdr) {
			node = FIELD_TO_STRUCTURE(entry, KXmlNode, entry);
			if (0 == strcmp(node->name, path))
				curIndex++;
			entry = entry->next;
		}
	}
	return curIndex;
}

/**
 * @brief Goto and return KXmlNode.
 *
 * @param doc KXmlDoc
 * @param path node path, only one level supported
 * - path == "/": goto Root node, index param ignored.
 * - path == ".": Current node, index param ignored.
 * - path == "..": Parent node, index param ignored.
 * - path == "": ignore name, get (index)th sub node.
 * - path == "ename": (index)th sub element name "ename".
 * @param index Return (index)th node.
 * - index >= 0: (index)th from first element
 * - index < 0: (index)th from last element with verse direction.
 *
 * @return KXmlNode for success, knil for error.
 */
KXmlNode *xmldoc_getNode(KXmlDoc *doc, const kchar *path, kint index)
{
	if (doc && path) {
		KXmlNode *curnode = doc->cur_node;

		K_dlist_entry *entry;
		KXmlNode *node;
		kint curIndex, anynode;

		if (0 == strcmp(path, "/"))
			/* root */
			return doc->root;
		else if (0 == strcmp(path, "."))
			/* current node */
			return doc->cur_node;
		else if (0 == strcmp(path, ".."))
			/* parent, only one level */
			if (doc->cur_node && doc->cur_node->parentNode)
				return doc->cur_node->parentNode;
			else
				return doc->root;

		/* if (path == "") mean walk the subs */
		anynode = !strcmp("", path);

		curIndex = 0;
		if (index >= 0) {
			entry = curnode->subHdr.next;
			while (entry != &curnode->subHdr) {
				node = FIELD_TO_STRUCTURE(entry, KXmlNode, entry);
				if (anynode || 0 == strcmp(node->name, path)) {
					if (curIndex == index)
						return node;
					curIndex++;
				}
				entry = entry->next;
			}
		} else {
			entry = curnode->subHdr.prev;
			while (entry != &curnode->subHdr) {
				node = FIELD_TO_STRUCTURE(entry, KXmlNode, entry);
				if (anynode || 0 == strcmp(node->name, path)) {
					if (curIndex == index)
						return node;
					curIndex--;
				}
				entry = entry->prev;
			}
		}
	}
	return knil;
}

/**
 * @brief Set current node
 *
 * @param doc KXmlDoc
 * @param path node name
 * @param index Goto (index)th of path node
 */
KXmlNode *xmldoc_gotoNode(KXmlDoc *doc, const kchar *path, kint index)
{
	/* similar to xmldoc_getNode, but it will set cur_node to path */
	KXmlNode *node = xmldoc_getNode(doc, path, index);
	if (node)
		doc->cur_node = node;
	return node;
}

/**
 * @brief Append a node to KXmlDoc
 *
 * @param doc KXmlDoc
 * @param node XML node to be append.
 * @param parent Parent node, if knil, default to current node
 */
KXmlNode *xmldoc_addNode(KXmlDoc *doc, KXmlNode *node, KXmlNode *parent)
{
	if (doc && node) {
		if (!parent)
			parent = doc->cur_node;
		insert_dlist_tail_entry(&parent->subHdr, &node->entry);
		node->parentNode = parent;
	}
	return node;
}

/**
 * @brief Construct a KXmlNode
 */
KXmlNode *xmlnode_new(KXmlNode *node, const kchar *name, const kchar *text)
{
	if (!node)
		node = (KXmlNode*)kmem_alloz(1, KXmlNode);

	if (node) {
		init_dlist_head(&node->entry);
		init_dlist_head(&node->subHdr);
		init_dlist_head(&node->attrHdr);
		node->parentNode = knil;
		node->name = name ? kstr_dup(name) : knil;
		node->text = text ? kstr_dup(text) : knil;
	}
	return node;
}

kint xmlnode_set_name(KXmlNode *node, const kchar *name)
{
	setVal(node->name, name);
	return 0;
}

kint xmlnode_set_text(KXmlNode *node, const kchar *text)
{
	setVal(node->text, text);
	return 0;
}

kint xmlnode_set_attr_value(KXmlNode *node, const kchar *name, const kchar *value)
{
	KXmlAttr *attr;
	attr = xmlnode_getattr(node, name);
	if (attr)
		setVal(attr->value, value);
	else {
		attr = xmlattr_new(knil, name, value);
		xmlnode_addattr(node, attr);
	}
	return 0;
}

kchar *xmlnode_get_attr_value(KXmlNode *node, const kchar *name)
{
	KXmlAttr *attr;
	attr = xmlnode_getattr(node, name);
	if (attr)
		return attr->value;
	return knil;
}

kint xmlnode_del(KXmlNode *node)
{
	if (node) {
		K_dlist_entry *entry;
		KXmlAttr *attr;
		KXmlNode *snode;

		while (!is_dlist_empty(&node->attrHdr)) {
			entry = remove_dlist_tail_entry(&node->attrHdr);
			attr = FIELD_TO_STRUCTURE(entry, KXmlAttr, entry);
			xmlattr_del(attr);
		}
		while (!is_dlist_empty(&node->subHdr)) {
			entry = remove_dlist_tail_entry(&node->subHdr);
			snode = FIELD_TO_STRUCTURE(entry, KXmlNode, entry);
			xmlnode_del(snode);
		}

		kmem_free(node->name);
		kmem_free(node->text);

		kmem_free(node);
		return 0;
	}
	return -1;
}

kint xmlnode_detach(KXmlNode *node)
{
	remove_dlist_entry(&node->entry);
	node->parentNode = knil;
	return 0;
}

/**
 * @brief Get Next sibling node
 *
 * @param node KXmlNode
 *
 * @return If current node is the last node, it return knil.
 * if error return -1, otherwise return the next node.
 *
 * @warning This function do not change current node to next
 */
KXmlNode *xmlnode_next(KXmlNode *node)
{
	kassert(node);
	if (node && node->parentNode)
		if (&node->parentNode->subHdr != node->entry.next)
			return FIELD_TO_STRUCTURE(node->entry.next, KXmlNode, entry);

	return knil;
}

KXmlNode *xmlnode_next_same(KXmlNode *node)
{
	KXmlNode *tnode = node;
	kassert(node);
	if (node && node->parentNode)
		while (&tnode->parentNode->subHdr != tnode->entry.next) {
			tnode = FIELD_TO_STRUCTURE(tnode->entry.next, KXmlNode, entry);
			if (!strcmp(tnode->name, node->name))
				return tnode;

		}

	return knil;
}

KXmlNode *xmlnode_prev(KXmlNode *node)
{
	kassert(node);
	if (node && node->parentNode)
		if (&node->parentNode->subHdr != node->entry.prev)
			return FIELD_TO_STRUCTURE(node->entry.prev, KXmlNode, entry);

	return knil;
}

KXmlNode *xmlnode_prev_same(KXmlNode *node)
{
	KXmlNode *tnode = node;
	kassert(node);
	if (node && node->parentNode)
		while (&tnode->parentNode->subHdr != tnode->entry.prev) {
			tnode = FIELD_TO_STRUCTURE(tnode->entry.prev, KXmlNode, entry);
			if (!strcmp(tnode->name, node->name))
				return tnode;
		}

	return knil;
}

kint xmlnode_addattr(KXmlNode *node, KXmlAttr *attr)
{
	if (node && attr) {
		insert_dlist_tail_entry(&node->attrHdr, &attr->entry);
		attr->parentNode = node;
		return 0;
	}
	return -1;
}

KXmlAttr *xmlnode_getattr(KXmlNode *node, const kchar *attrname)
{
	if (node && attrname) {
		K_dlist_entry *entry;
		KXmlAttr *attr;
		entry = node->attrHdr.next;
		while (entry != &node->attrHdr) {
			attr = FIELD_TO_STRUCTURE(entry, KXmlAttr, entry);
			if (0 == strcmp(attr->name, attrname))
				return attr;
			entry = entry->next;
		}
	}
	return knil;
}

KXmlAttr *xmlattr_new(KXmlAttr *attr, const kchar *name, const kchar *value)
{
	if (!attr)
		attr = (KXmlAttr*)kmem_alloz(1, KXmlAttr);

	if (attr) {
		init_dlist_head(&attr->entry);
		attr->parentNode = knil;
		setVal(attr->name, name);
		setVal(attr->value, value);
	}
	return attr;
}

kint xmlattr_del(KXmlAttr *attr)
{
	if (attr) {
		kmem_free(attr->name);
		kmem_free(attr->value);
		kmem_free(attr);
		return 0;
	}
	return -1;
}

kint xmlattr_detach(KXmlAttr *attr)
{
	remove_dlist_entry(&attr->entry);
	attr->parentNode = knil;
	return 0;
}

kint xmlattr_set_name(KXmlAttr *attr, const kchar *name)
{
	setVal(attr->name, name);
	return 0;
}

kint xmlattr_set_value(KXmlAttr *attr, const kchar *value)
{
	setVal(attr->value, value);
	return 0;
}

#ifdef WIN33
kint xsmain(kint argc, kchar **argv)
{
	kint depth = 0, len;
	kchar buffer[81920];
	kbean fp = knil;
	KXmlDoc *doc = xmldoc_new(knil);
	KXmlNode *node;
	KXmlAttr *attr;

	if (argc < 2)
		return -1;

	fp = fopen(argv[1], "rt");
	if (!fp)
		return -1;

	len = fread(buffer, 1, sizeof(buffer), fp);
	buffer[len] = '\0';

	xmldoc_parse(doc, buffer, len);

	node = xmldoc_getNode(doc, ".", 0);
	node = xmldoc_gotoNode(doc, "/", 1);
	node = xmldoc_gotoNode(doc, "Service", 0);
	node = xmldoc_gotoNode(doc, ".", 0);
	node = xmldoc_gotoNode(doc, "Name", 2);
	attr = xmlnode_getattr(node, "lang");
	node = xmldoc_gotoNode(doc, "..", 2);

	xmldoc_save(doc, "saved.xml");

	xmldoc_del(doc);

	getchar();
}
#endif
