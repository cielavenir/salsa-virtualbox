/** @file
 * VirtualBox File System for Solaris Guests, vnode implementation.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * Sun Microsystems, Inc. confidential
 * All rights reserved
 */

/*
 * Shared Folder File System is used from Solaris when run as a guest operating
 * system on VirtualBox, though is meant to be usable with any hypervisor that
 * can provide similar functionality. The sffs code handles all the Solaris
 * specific semantics and relies on a provider module to actually access
 * directories, files, etc. The provider interfaces are described in
 * "vboxfs_prov.h" and the module implementing them is shipped as part of the
 * VirtualBox Guest Additions for Solaris.
 *
 * The shared folder file system is similar to a networked file system,
 * but with some caveats. The sffs code caches minimal information and proxies
 * out to the provider whenever possible. Here are some things that are
 * handled in this code and not by the proxy:
 *
 * - a way to open ".." from any already open directory
 * - st_ino numbers
 * - detecting directory changes that happened on the host.
 *
 * The implementation builds a cache of information for every file/directory
 * ever accessed in all mounted sffs filesystems using sf_node structures.
 *
 * This information for both open or closed files can become invalid if
 * asynchronous changes are made on the host. Solaris should not panic() in
 * this event, but some file system operations may return unexpected errors.
 * Information for such directories or files while they have active vnodes
 * is removed from the regular cache and stored in a "stale" bucket until
 * the vnode becomes completely inactive.
 *
 * No file data is cached in the guest. This means we don't support mmap() yet.
 * A future version could relatively easily add support for read-only
 * mmap(MAP_SHARED) and any mmap(MAP_PRIVATE). But a la ZFS, this data caching
 * would not be coherent with normal simultaneous read()/write() operations,
 * nor will it be coherent with data access on the host. Writable
 * mmap(MAP_SHARED) access is possible, but guaranteeing any kind of coherency
 * with concurrent activity on the host would be near impossible with the
 * existing interfaces.
 *
 * A note about locking. sffs is not a high performance file system.
 * No fine grained locking is done. The one sffs_lock protects just about
 * everything.
 */

#include <VBox/log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/modctl.h>
#include <sys/mount.h>
#include <sys/policy.h>
#include <sys/atomic.h>
#include <sys/sysmacros.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/vfs.h>
#if !defined(VBOX_VFS_SOLARIS_10U6)
#include <sys/vfs_opreg.h>
#endif
#include <sys/pathname.h>
#include <sys/dirent.h>
#include <sys/fs_subr.h>
#include "vboxfs_prov.h"
#include "vboxfs_vnode.h"
#include "vboxfs_vfs.h"

static struct vnodeops *sffs_ops = NULL;

kmutex_t sffs_lock;
static avl_tree_t sfnodes;
static avl_tree_t stale_sfnodes;

/*
 * For now we'll use an I/O buffer that doesn't page fault for VirtualBox
 * to transfer data into.
 */
char *sffs_buffer;

/*
 * sfnode_compare() is needed for AVL tree functionality.
 * The nodes are sorted by mounted filesystem, then path. If the
 * nodes are stale, the node pointer itself is used to force uniqueness.
 */
static int
sfnode_compare(const void *a, const void *b)
{
	sfnode_t *x = (sfnode_t *)a;
	sfnode_t *y = (sfnode_t *)b;
	int diff;

	if (x->sf_is_stale) {
		ASSERT(y->sf_is_stale);
		diff = strcmp(x->sf_path, y->sf_path);
		if (diff == 0)
			diff = (uintptr_t)y - (uintptr_t)x;
	} else {
		ASSERT(!y->sf_is_stale);
		diff = (uintptr_t)y->sf_sffs - (uintptr_t)x->sf_sffs;
		if (diff == 0)
			diff = strcmp(x->sf_path, y->sf_path);
	}
	if (diff < 0)
		return (-1);
	if (diff > 0)
		return (1);
	return (0);
}

/*
 * Construct a new pathname given an sfnode plus an optional tail component.
 * This handles ".." and "."
 */
static char *
sfnode_construct_path(sfnode_t *node, char *tail)
{
	char *p;

	if (strcmp(tail, ".") == 0 || strcmp(tail, "..") == 0)
		panic("construct path for %s", tail);
	p = kmem_alloc(strlen(node->sf_path) + 1 + strlen(tail) + 1, KM_SLEEP);
	strcpy(p, node->sf_path);
	strcat(p, "/");
	strcat(p, tail);
	return (p);
}

/*
 * Open the provider file associated with a vnode. Holding the file open is
 * the only way we have of trying to have a vnode continue to refer to the
 * same host file in the host in light of the possibility of host side renames.
 */
static void
sfnode_open(sfnode_t *node)
{
	int error;
	sfp_file_t *fp;

	if (node->sf_file != NULL)
		return;
	error = sfprov_open(node->sf_sffs->sf_handle, node->sf_path, &fp);
	if (error == 0)
		node->sf_file = fp;
}

/*
 * get a new vnode reference for an sfnode
 */
vnode_t *
sfnode_get_vnode(sfnode_t *node)
{
	vnode_t *vp;

	if (node->sf_vnode != NULL) {
		VN_HOLD(node->sf_vnode);
	} else {
		vp = vn_alloc(KM_SLEEP);
        LogFlowFunc(("  %s gets vnode 0x%p\n", node->sf_path, vp));
		vp->v_type = node->sf_type;
		vp->v_vfsp = node->sf_sffs->sf_vfsp;
		VFS_HOLD(vp->v_vfsp);
		vn_setops(vp, sffs_ops);
		vp->v_flag = VNOMAP | VNOSWAP;
		vn_exists(vp);
		vp->v_data = node;
		node->sf_vnode = vp;
	}
	sfnode_open(node);
	return (node->sf_vnode);
}

/*
 * Allocate and initialize a new sfnode and assign it a vnode
 */
sfnode_t *
sfnode_make(
	sffs_data_t	*sffs,
	char		*path,
	vtype_t		type,
	sfp_file_t	*fp,
	sfnode_t	*parent)	/* can be NULL for root */
{
	sfnode_t	*node;
	avl_index_t	where;

	ASSERT(MUTEX_HELD(&sffs_lock));
	ASSERT(path != NULL);

	/*
	 * build the sfnode
	 */
    LogFlowFunc(("sffs_make(%s)\n", path));
	node = kmem_alloc(sizeof (*node), KM_SLEEP);
	node->sf_sffs = sffs;
	node->sf_path = path;
	node->sf_ino = sffs->sf_ino++;
	node->sf_type = type;
	node->sf_is_stale = 0;	/* never stale at creation */
	node->sf_file = fp;
	node->sf_vnode = NULL;	/* do this before any sfnode_get_vnode() */
	node->sf_children = 0;
	node->sf_parent = parent;
	if (parent)
		++parent->sf_children;

	/*
	 * add the new node to our cache
	 */
	if (avl_find(&sfnodes, node, &where) != NULL)
		panic("sffs_create_sfnode(%s): duplicate sfnode_t", path);
	avl_insert(&sfnodes, node, where);
	return (node);
}

/*
 * destroy an sfnode
 */
static void
sfnode_destroy(sfnode_t *node)
{
	avl_index_t where;
	avl_tree_t *tree;
	sfnode_t *parent;
top:
	parent = node->sf_parent;
	ASSERT(MUTEX_HELD(&sffs_lock));
	ASSERT(node->sf_path != NULL);
    LogFlowFunc(("sffs_destroy(%s)%s\n", node->sf_path, node->sf_is_stale ? " stale": ""));
	if (node->sf_children != 0)
		panic("sfnode_destroy(%s) has %d children", node->sf_children);
	if (node->sf_vnode != NULL)
		panic("sfnode_destroy(%s) has active vnode", node->sf_path);

	if (node->sf_is_stale)
		tree = &stale_sfnodes;
	else
		tree = &sfnodes;
	if (avl_find(tree, node, &where) == NULL)
		panic("sfnode_destroy(%s) not found", node->sf_path);
	avl_remove(tree, node);

	VFS_RELE(node->sf_sffs->sf_vfsp);
	kmem_free(node->sf_path, strlen(node->sf_path) + 1);
	kmem_free(node, sizeof (*node));
	if (parent != NULL) {
		if (parent->sf_children == 0)
			panic("sfnode_destroy(%s) parent has no child");
		--parent->sf_children;
		if (parent->sf_children == 0 &&
		    parent->sf_is_stale &&
		    parent->sf_vnode == NULL) {
			node = parent;
			goto top;
		}
	}
}

/*
 * Some sort of host operation on an sfnode has failed or it has been
 * deleted. Mark this node and any children as stale, deleting knowledge
 * about any which do not have active vnodes or children
 * This also handle deleting an inactive node that was already stale.
 */
static void
sfnode_make_stale(sfnode_t *node)
{
	sfnode_t *n;
	int len;
	ASSERT(MUTEX_HELD(&sffs_lock));
	avl_index_t where;

	/*
	 * First deal with any children of a directory node.
	 * If a directory becomes stale, anything below it becomes stale too.
	 */
	if (!node->sf_is_stale && node->sf_type == VDIR) {
		len = strlen(node->sf_path);
		
		n = node;
		while ((n = AVL_NEXT(&sfnodes, node)) != NULL) {
			ASSERT(!n->sf_is_stale);

			/*
			 * quit when no longer seeing children of node
			 */
			if (n->sf_sffs != node->sf_sffs ||
			    strncmp(node->sf_path, n->sf_path, len) != 0 ||
			    n->sf_path[len] != '/')
				break;

			/*
			 * Either mark the child as stale or destroy it
			 */
			if (n->sf_vnode == NULL && n->sf_children == 0) {
				sfnode_destroy(n);
			} else {
                LogFlowFunc(("sffs_make_stale(%s) sub\n", n->sf_path));
				if (avl_find(&sfnodes, n, &where) == NULL)
					panic("sfnode_make_stale(%s)"
					    " not in sfnodes", n->sf_path);
				avl_remove(&sfnodes, n);
				n->sf_is_stale = 1;
				if (avl_find(&stale_sfnodes, n, &where) != NULL)
					panic("sffs_make_stale(%s) duplicates",
					    n->sf_path);
				avl_insert(&stale_sfnodes, n, where);
			}
		}
	}

	/*
	 * Now deal with the given node.
	 */
	if (node->sf_vnode == NULL && node->sf_children == 0) {
		sfnode_destroy(node);
	} else if (!node->sf_is_stale) {
        LogFlowFunc(("sffs_make_stale(%s)\n", node->sf_path));
		if (avl_find(&sfnodes, node, &where) == NULL)
			panic("sfnode_make_stale(%s) not in sfnodes",
			    node->sf_path);
		avl_remove(&sfnodes, node);
		node->sf_is_stale = 1;
		if (avl_find(&stale_sfnodes, node, &where) != NULL)
			panic("sffs_make_stale(%s) duplicates", node->sf_path);
		avl_insert(&stale_sfnodes, node, where);
	}
}

/*
 * Rename a file or a directory
 */
static void
sfnode_rename(sfnode_t *node, sfnode_t *newparent, char *path)
{
	sfnode_t *n;
	sfnode_t template;
	avl_index_t where;
	int len = strlen(path);
	int old_len;
	char *new_path;
	char *tail;
	ASSERT(MUTEX_HELD(&sffs_lock));

	ASSERT(!node->sf_is_stale);

	/*
	 * Have to remove anything existing that had the new name.
	 */
	template.sf_sffs = node->sf_sffs;
	template.sf_path = path;
	template.sf_is_stale = 0;
	n = avl_find(&sfnodes, &template, &where);
	if (n != NULL)
		sfnode_make_stale(n);

	/*
	 * Do the renaming, deal with any children of this node first.
	 */
	if (node->sf_type == VDIR) {
		old_len = strlen(node->sf_path);
		while ((n = AVL_NEXT(&sfnodes, node)) != NULL) {

			/*
			 * quit when no longer seeing children of node
			 */
			if (n->sf_sffs != node->sf_sffs ||
			    strncmp(node->sf_path, n->sf_path, old_len) != 0 ||
			    n->sf_path[old_len] != '/')
				break;

			/*
			 * Rename the child:
			 * - build the new path name
			 * - unlink the AVL node
			 * - assign the new name
			 * - re-insert the AVL name
			 */
			ASSERT(strlen(n->sf_path) > old_len);
			tail = n->sf_path + old_len; /* includes intial "/" */
			new_path = kmem_alloc(len + strlen(tail) + 1,
			    KM_SLEEP);
			strcpy(new_path, path);
			strcat(new_path, tail);
			if (avl_find(&sfnodes, n, &where) == NULL)
				panic("sfnode_rename(%s) not in sfnodes",
				   n->sf_path);
			avl_remove(&sfnodes, n);
            LogFlowFunc(("sfnode_rname(%s to %s) sub\n", n->sf_path, new_path));
			kmem_free(n->sf_path, strlen(n->sf_path) + 1);
			n->sf_path = new_path;
			if (avl_find(&sfnodes, n, &where) != NULL)
				panic("sfnode_rename(%s) duplicates",
				    n->sf_path);
			avl_insert(&sfnodes, n, where);
		}
	}

	/*
	 * Deal with the given node.
	 */
	if (avl_find(&sfnodes, node, &where) == NULL)
		panic("sfnode_rename(%s) not in sfnodes", node->sf_path);
	avl_remove(&sfnodes, node);
    LogFlowFunc(("sfnode_rname(%s to %s)\n", node->sf_path, path));
	kmem_free(node->sf_path, strlen(node->sf_path) + 1);
	node->sf_path = path;
	if (avl_find(&sfnodes, node, &where) != NULL)
		panic("sfnode_rename(%s) duplicates", node->sf_path);
	avl_insert(&sfnodes, node, where);

	/*
	 * change the parent
	 */
	if (node->sf_parent == NULL)
		panic("sfnode_rename(%s) no parent", node->sf_path);
	if (node->sf_parent->sf_children == 0)
		panic("sfnode_rename(%s) parent has no child", node->sf_path);
	--node->sf_parent->sf_children;
	node->sf_parent = newparent;
	++newparent->sf_children;
}

/*
 * Look for a cached node, if not found either handle ".." or try looking
 * via the provider. Create an entry in sfnodes if found but not cached yet.
 * If the create flag is set, a file or directory is created. If the file
 * already existed, an error is returned.
 * Nodes returned from this routine always have a vnode with its ref count
 * bumped by 1.
 */
static sfnode_t *
sfnode_lookup(sfnode_t *dir, char *name, vtype_t create)
{
	avl_index_t	where;
	sfnode_t	template;
	sfnode_t	*node;
	int		error;
	int		type;
	char		*fullpath;
	sfp_file_t	*fp;

	ASSERT(MUTEX_HELD(&sffs_lock));

	/*
	 * handle referencing myself
	 */
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0)
		return (dir);

	/*
	 * deal with parent
	 */
	if (strcmp(name, "..") == 0)
		return (dir->sf_parent);

	/*
	 * Look for an existing node. 
	 */
	fullpath = sfnode_construct_path(dir, name);
	template.sf_sffs = dir->sf_sffs;
	template.sf_path = fullpath;
	template.sf_is_stale = 0;
	node = avl_find(&sfnodes, &template, &where);
	if (node != NULL) {
		kmem_free(fullpath, strlen(fullpath) + 1);
		if (create != VNON)
			return (NULL);
		return (node);
	}

	/*
	 * No entry for this path currently.
	 * Check if the file exists with the provider and get the type from
	 * there.
	 */
	if (create == VREG) {
		type = VREG;
		error = sfprov_create(dir->sf_sffs->sf_handle, fullpath, &fp);
	} else if (create == VDIR) {
		type = VDIR;
		error = sfprov_mkdir(dir->sf_sffs->sf_handle, fullpath, &fp);
	} else {
		mode_t m;
		fp = NULL;
		type = VNON;
		error =
		    sfprov_get_mode(dir->sf_sffs->sf_handle, fullpath, &m);
		if (error != 0)
			error = ENOENT;
		else if (S_ISDIR(m))
			type = VDIR;
		else if (S_ISREG(m))
			type = VREG;
	}

	/*
	 * If no errors, make a new node and return it.
	 */
	if (error) {
		kmem_free(fullpath, strlen(fullpath) + 1);
		return (NULL);
	}
	node = sfnode_make(dir->sf_sffs, fullpath, type, fp, dir);
	return (node);
}


/*
 * uid and gid in sffs determine owner and group for all files.
 */
static int
sfnode_access(sfnode_t *node, mode_t mode, cred_t *cr)
{
	sffs_data_t *sffs = node->sf_sffs;
	mode_t m;
	int shift = 0;
	int error;
	vnode_t *vp;

	ASSERT(MUTEX_HELD(&sffs_lock));

	/*
	 * get the mode from the provider
	 */
	error = sfprov_get_mode(node->sf_sffs->sf_handle, node->sf_path, &m);
	if (error != 0) {
		m = 0;
		if (error == ENOENT)
			sfnode_make_stale(node);
	}

	/*
	 * mask off the permissions based on uid/gid
	 */
	if (crgetuid(cr) != sffs->sf_uid) {
		shift += 3;
		if (groupmember(sffs->sf_gid, cr) == 0)
			shift += 3;
	}
	mode &= ~(m << shift);

	if (mode == 0) {
		error = 0;
	} else {
		vp = sfnode_get_vnode(node);
		error = secpolicy_vnode_access(cr, vp, sffs->sf_uid, mode);
		VN_RELE(vp);
	}
	return (error);
}


/*
 *
 * Everything below this point are the vnode operations used by Solaris VFS
 */
static int
sffs_readdir(
	vnode_t		*vp,
	uio_t		*uiop,
	cred_t		*cred,
	int		*eofp,
	caller_context_t *ct,
	int		flags)
{
	sfnode_t *dir = VN2SFN(vp);
	sfnode_t *node;
	struct dirent64 *dirent;
	int dummy_eof;
	int error = 0;
	int namelen;
	void *prov_buff = NULL;
	size_t prov_buff_size;
	char **names;
	uint32_t nents;
	uint32_t index;

	if (uiop->uio_iovcnt != 1)
		return (EINVAL);

	if (vp->v_type != VDIR)
		return (ENOTDIR);

	if (eofp == NULL)
		eofp = &dummy_eof;
	*eofp = 0;

	if (uiop->uio_loffset >= MAXOFF_T) {
		*eofp = 1;
		return (0);
	}

	dirent = kmem_zalloc(DIRENT64_RECLEN(MAXNAMELEN), KM_SLEEP);

	/*
	 * Get the directory entry names from the host. This gets all
	 * entries, so add in starting offset. Max the caller can expect
	 * would be the size of the UIO buffer / sizeof of a dirent for
	 * file with name of length 1
	 */
	mutex_enter(&sffs_lock);
	index = uiop->uio_loffset;
	nents = index + (uiop->uio_resid / DIRENT64_RECLEN(1));
	error = sfprov_readdir(dir->sf_sffs->sf_handle, dir->sf_path,
	    &prov_buff, &prov_buff_size, &nents);
	if (error != 0)
		goto done;
	if (nents <= index) {
		*eofp = 1;
		goto done;
	}
	names = (void *)prov_buff;
	
	/*
	 * Lookup each of the names, so that we have ino's.
	 */
	for (; index < nents; ++index) {
		if (strcmp(names[index], ".") == 0) {
			node = dir;
		} else if (strcmp(names[index], "..") == 0) {
			node = dir->sf_parent;
			if (node == NULL)
				node = dir;
		} else {
			node = sfnode_lookup(dir, names[index], VNON);
			if (node == NULL)
				panic("sffs_readdir() lookup failed");
		}
		namelen = strlen(names[index]);
		strcpy(&dirent->d_name[0], names[index]);
		dirent->d_reclen = DIRENT64_RECLEN(namelen);
		dirent->d_off = index;
		dirent->d_ino = node->sf_ino;
		if (dirent->d_reclen > uiop->uio_resid) {
			error = ENOSPC;
			break;
		}
		error = uiomove(dirent, dirent->d_reclen, UIO_READ, uiop);
		if (error != 0)
			break;
		bzero(&dirent->d_name[0], namelen);
	}
	if (error == 0 && index >= nents)
		*eofp = 1;
done:
	mutex_exit(&sffs_lock);
	if (prov_buff != NULL)
		kmem_free(prov_buff, prov_buff_size);
	kmem_free(dirent, DIRENT64_RECLEN(MAXNAMELEN));
	return (error);
}


#if defined(VBOX_VFS_SOLARIS_10U6)
/*
 * HERE JOE.. this may need more logic, need to look at other file systems
 */
static int
sffs_pathconf(
	vnode_t	*vp,
	int	cmd,
	ulong_t	*valp,
	cred_t	*cr)
{
	return (fs_pathconf(vp, cmd, valp, cr));
}
#else
/*
 * HERE JOE.. this may need more logic, need to look at other file systems
 */
static int
sffs_pathconf(
	vnode_t	*vp,
	int	cmd,
	ulong_t	*valp,
	cred_t	*cr,
	caller_context_t *ct)
{
	return (fs_pathconf(vp, cmd, valp, cr, ct));
}
#endif

static int
sffs_getattr(
	vnode_t		*vp,
	vattr_t		*vap,
	int		flags,
	cred_t		*cred,
	caller_context_t *ct)
{
	sfnode_t	*node = VN2SFN(vp);
	sffs_data_t	*sffs = node->sf_sffs;
	mode_t		mode;
	timestruc_t	time;
	uint64_t	x;
	int		error;

	mutex_enter(&sffs_lock);
	vap->va_type = vp->v_type;
	vap->va_uid = sffs->sf_uid;
	vap->va_gid = sffs->sf_gid;
	vap->va_fsid = sffs->sf_vfsp->vfs_dev;
	vap->va_nodeid = node->sf_ino;
	vap->va_nlink = 1;
	vap->va_rdev =  sffs->sf_vfsp->vfs_dev;
	vap->va_seq = 0;

	error = sfprov_get_mode(node->sf_sffs->sf_handle, node->sf_path, &mode);
	if (error == ENOENT)
		sfnode_make_stale(node);
	if (error != 0)
		goto done;
	vap->va_mode = mode & MODEMASK;

	error = sfprov_get_size(node->sf_sffs->sf_handle, node->sf_path, &x);
	if (error == ENOENT)
		sfnode_make_stale(node);
	if (error != 0)
		goto done;
	vap->va_size = x;
	vap->va_blksize = 512;
	vap->va_nblocks = (x + 511) / 512;

	error =
	    sfprov_get_atime(node->sf_sffs->sf_handle, node->sf_path, &time);
	if (error == ENOENT)
		sfnode_make_stale(node);
	if (error != 0)
		goto done;
	vap->va_atime = time;

	error =
	    sfprov_get_mtime(node->sf_sffs->sf_handle, node->sf_path, &time);
	if (error == ENOENT)
		sfnode_make_stale(node);
	if (error != 0)
		goto done;
	vap->va_mtime = time;

	error =
	    sfprov_get_ctime(node->sf_sffs->sf_handle, node->sf_path, &time);
	if (error == ENOENT)
		sfnode_make_stale(node);
	if (error != 0)
		goto done;
	vap->va_ctime = time;

done:
	mutex_exit(&sffs_lock);
	return (error);
}

/*ARGSUSED*/
static int
sffs_read(
	vnode_t		*vp,
	struct uio	*uio,
	int		ioflag,
	cred_t		*cred,
	caller_context_t *ct)
{
	sfnode_t	*node = VN2SFN(vp);
	int		error = 0;
	uint32_t	bytes;
	uint32_t	done;
	ulong_t 	offset;
	ssize_t		total;

	if (vp->v_type == VDIR)
		return (EISDIR);
	if (vp->v_type != VREG)
		return (EINVAL);
	if (uio->uio_loffset >= MAXOFF_T)
       		return (0);
        if (uio->uio_loffset < 0)
       		return (EINVAL);
        total = uio->uio_resid;
        if (total == 0)
       		return (0);

	mutex_enter(&sffs_lock);
	sfnode_open(node);
	if (node->sf_file == NULL) {
		mutex_exit(&sffs_lock);
		return (EINVAL);
	}

	do {
		offset = uio->uio_offset;
		done = bytes = MIN(PAGESIZE, uio->uio_resid);
		error = sfprov_read(node->sf_file, sffs_buffer, offset, &done);
		if (error == 0 && done > 0)
			error = uiomove(sffs_buffer, done, UIO_READ, uio);
	} while (error == 0 && uio->uio_resid > 0 && done > 0);

	mutex_exit(&sffs_lock);

	/*
	 * a partial read is never an error
	 */
	if (total != uio->uio_resid)
		error = 0;
	return (error);
}

/*ARGSUSED*/
static int
sffs_write(
	vnode_t		*vp,
	struct uio	*uiop,
	int		ioflag,
	cred_t		*cred,
	caller_context_t *ct)
{
	sfnode_t	*node = VN2SFN(vp);
	int		error = 0;
	uint32_t	bytes;
	uint32_t	done;
	ulong_t 	offset;
	ssize_t		total;
	rlim64_t	limit = uiop->uio_llimit;

	if (vp->v_type == VDIR)
		return (EISDIR);
	if (vp->v_type != VREG)
		return (EINVAL);

	/*
	 * We have to hold this lock for a long time to keep
	 * multiple FAPPEND writes from intermixing
	 */
	mutex_enter(&sffs_lock);
	sfnode_open(node);
	if (node->sf_file == NULL) {
		mutex_exit(&sffs_lock);
		return (EINVAL);
	}
	if (ioflag & FAPPEND) {
		uint64_t endoffile;

		error = sfprov_get_size(node->sf_sffs->sf_handle,
		    node->sf_path, &endoffile);
		if (error == ENOENT)
			sfnode_make_stale(node);
		if (error != 0) {
			mutex_exit(&sffs_lock);
			return (error);
		}
		uiop->uio_loffset = endoffile;
	}

	if (vp->v_type != VREG || uiop->uio_loffset < 0) {
		mutex_exit(&sffs_lock);
       		return (EINVAL);
	}
	if (limit == RLIM64_INFINITY || limit > MAXOFFSET_T)
                limit = MAXOFFSET_T;
	if (limit > MAXOFF_T)
                limit = MAXOFF_T;

	if (uiop->uio_loffset >= limit) {
		proc_t *p = ttoproc(curthread);
		mutex_enter(&p->p_lock);
		(void) rctl_action(rctlproc_legacy[RLIMIT_FSIZE], p->p_rctls,
		    p, RCA_UNSAFE_SIGINFO);
		mutex_exit(&p->p_lock);
		mutex_exit(&sffs_lock);
		return (EFBIG);
	}

	if (uiop->uio_loffset >= MAXOFF_T) {
		mutex_exit(&sffs_lock);
		return (EFBIG);
	}


        total = uiop->uio_resid;
        if (total == 0) {
		mutex_exit(&sffs_lock);
       		return (0);
	}

	do {
		offset = uiop->uio_offset;
		bytes = MIN(PAGESIZE, uiop->uio_resid);
		if (offset + bytes >= limit) {
                	if (offset >= limit) {
                       		error = EFBIG;
                       		break;
                        }
                        bytes = limit - offset;
		}
		error = uiomove(sffs_buffer, bytes, UIO_WRITE, uiop);
		if (error != 0)
			break;
		done = bytes;
		if (error == 0)
			error = sfprov_write(node->sf_file, sffs_buffer,
			    offset, &done);
		total -= done;
		if (done != bytes) {
			uiop->uio_resid += bytes - done;
			break;
		}
	} while (error == 0 && uiop->uio_resid > 0 && done > 0);

	mutex_exit(&sffs_lock);

	/*
	 * A short write is never really an error.
	 */
	if (total != uiop->uio_resid)
		error = 0;
	return (error);
}

/*ARGSUSED*/
static int
sffs_access(vnode_t *vp, int mode, int flags, cred_t *cr, caller_context_t *ct)
{
	sfnode_t *node = VN2SFN(vp);
	int error;

	mutex_enter(&sffs_lock);
	error = sfnode_access(node, mode, cr);
	mutex_exit(&sffs_lock);
	return (error);
}

/*
 * Lookup an entry in a directory and create a new vnode if found.
 */
/* ARGSUSED3 */
static int
sffs_lookup(
	vnode_t		*dvp,		/* the directory vnode */
	char		*name,		/* the name of the file or directory */
	vnode_t		**vpp,		/* the vnode we found or NULL */
	struct pathname	*pnp,
	int		flags,
	vnode_t		*rdir,
	cred_t		*cred,
	caller_context_t *ct,
	int		*direntflags,
	struct pathname	*realpnp)
{
	int		error;
	sfnode_t	*node;

	/*
	 * dvp must be a directory
	 */
	if (dvp->v_type != VDIR)
		return (ENOTDIR);

	/*
	 * An empty component name or just "." means the directory itself.
	 * Don't do any further lookup or checking.
	 */
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0) {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	/*
	 * Check permission to look at this directory. We always allow "..".
	 */
	mutex_enter(&sffs_lock);
	if (strcmp(name, "..") != 0) {
		error = sfnode_access(VN2SFN(dvp), VEXEC, cred);
		if (error) {
			mutex_exit(&sffs_lock);
			return (error);
		}
	}

	/*
	 * Lookup the node.
	 */
	node = sfnode_lookup(VN2SFN(dvp), name, VNON);
	if (node != NULL)
		*vpp = sfnode_get_vnode(node);
	mutex_exit(&sffs_lock);
	return ((node == NULL) ? ENOENT : 0);
}

/*ARGSUSED*/
static int
sffs_create(
        vnode_t		*dvp,
        char		*name,
        struct vattr	*vap,
        vcexcl_t	exclusive,
        int		mode,
        vnode_t		**vpp,
        cred_t		*cr,
        int		flag,
        caller_context_t *ct,
        vsecattr_t	*vsecp)
{
	vnode_t		*vp;
	sfnode_t	*node;
	int		error;

	ASSERT(name != NULL);

	/*
	 * this is used for regular files, not mkdir
	 */
	if (vap->va_type == VDIR)
		return (EISDIR);
	if (vap->va_type != VREG)
		return (EINVAL);

	/*
	 * is this a pre-existing file?
	 */
	error = sffs_lookup(dvp, name, &vp,
	    NULL, 0, NULL, cr, ct, NULL, NULL);
	if (error == ENOENT)
		vp = NULL;
	else if (error != 0)
		return (error);

	/*
	 * Operation on a pre-existing file.
	 */
	if (vp != NULL) {
		if (exclusive == EXCL) {
			VN_RELE(vp);
			return (EEXIST);
		}
		if (vp->v_type == VDIR && (mode & VWRITE) == VWRITE) {
			VN_RELE(vp);
			return (EISDIR);
		}

		mutex_enter(&sffs_lock);
		node = VN2SFN(vp);
		error = sfnode_access(node, mode, cr);
		if (error != 0) {
			mutex_exit(&sffs_lock);
			VN_RELE(vp);
			return (error);
		}

		/*
		 * handle truncating an existing file
		 */
		if (vp->v_type == VREG && (vap->va_mask & AT_SIZE) &&
		    vap->va_size == 0) {
			sfnode_open(node);
			if (node->sf_path == NULL)
				error = ENOENT;
			else
				error = sfprov_trunc(node->sf_sffs->sf_handle,
				    node->sf_path);
			if (error) {
				mutex_exit(&sffs_lock);
				VN_RELE(vp);
				return (error);
			}
		}
		mutex_exit(&sffs_lock);
		*vpp = vp;
		return (0);
	}

	/*
	 * Create a new node. First check for a race creating it.
	 */
	mutex_enter(&sffs_lock);
	node = sfnode_lookup(VN2SFN(dvp), name, VNON);
	if (node != NULL) {
		mutex_exit(&sffs_lock);
		return (EEXIST);
	}

	/*
	 * Doesn't exist yet and we have the lock, so create it.
	 */
	node = sfnode_lookup(VN2SFN(dvp), name, VREG);
	mutex_exit(&sffs_lock);
	if (node == NULL)
		return (EINVAL);
	*vpp = sfnode_get_vnode(node);
	return (0);
}

/*ARGSUSED*/
static int
sffs_mkdir(
	vnode_t		*dvp,
	char		*nm,
	vattr_t		*va,
	vnode_t		**vpp,
	cred_t		*cred,
	caller_context_t *ct,
	int		flags,
	vsecattr_t	*vsecp)
{
	sfnode_t	*node;
	vnode_t		*vp;
	int		error;

	/*
	 * These should never happen
	 */
	ASSERT(nm != NULL);
	ASSERT(strcmp(nm, "") != 0);
	ASSERT(strcmp(nm, ".") != 0);
	ASSERT(strcmp(nm, "..") != 0);

	/*
	 * Do an unlocked look up first
	 */
	error = sffs_lookup(dvp, nm, &vp, NULL, 0, NULL, cred, ct, NULL, NULL);
	if (error == 0) {
		VN_RELE(vp);
		return (EEXIST);
	}
	if (error != ENOENT)
		return (error);

	/*
	 * Must be able to write in current directory
	 */
	mutex_enter(&sffs_lock);
	error = sfnode_access(VN2SFN(dvp), VWRITE, cred);
	if (error) {
		mutex_exit(&sffs_lock);
		return (error);
	}

	node = sfnode_lookup(VN2SFN(dvp), nm, VDIR);
	mutex_exit(&sffs_lock);
	if (node == NULL)
		return (EACCES);
	*vpp = sfnode_get_vnode(node);
	return (0);
}

/*ARGSUSED*/
static int
sffs_rmdir(
	struct vnode	*dvp,
	char		*nm,
	vnode_t		*cdir,
	cred_t		*cred,
	caller_context_t *ct,
	int		flags)
{
	sfnode_t	 *node;
	vnode_t		*vp;
	int		error;

	/*
	 * Return error when removing . and ..
	 */
	if (strcmp(nm, ".") == 0 || strcmp(nm, "") == 0)
		return (EINVAL);
	if (strcmp(nm, "..") == 0)
		return (EEXIST);

	error = sffs_lookup(dvp, nm, &vp, NULL, 0, NULL, cred, ct, NULL, NULL);
	if (error)
		return (error);
	if (vp->v_type != VDIR) {
		VN_RELE(vp);
		return (ENOTDIR);
	}
	if (vn_vfswlock(vp) || vn_mountedvfs(vp)) {
		VN_RELE(vp);
		return (EBUSY);
	}

	node = VN2SFN(vp);

	mutex_enter(&sffs_lock);
	error = sfnode_access(VN2SFN(dvp), VEXEC | VWRITE, cred);
	if (error)
		goto done;

	/*
	 * If anything else is using this vnode, then fail the remove.
	 * Why?  Windows hosts can't remove something that is open,
	 * so we have to sfprov_close() it first.
	 * There is no errno for this - since it's not a problem on UNIX,
	 * but EINVAL is the closest.
	 */
	if (node->sf_file != NULL) {
		if (vp->v_count > 1) {
			error = EINVAL;
			goto done;
		}
		(void)sfprov_close(node->sf_file);
		node->sf_file = NULL;
	}

	/*
	 * Remove the directory on the host and mark the node as stale.
	 */
	error = sfprov_rmdir(node->sf_sffs->sf_handle, node->sf_path);
	if (error == ENOENT || error == 0)
		sfnode_make_stale(node);
done:
	mutex_exit(&sffs_lock);
	VN_RELE(vp);
	return (error);
}


/*ARGSUSED*/
static int
sffs_remove(
	vnode_t		*dvp,
	char		*name,
	cred_t		*cred,
	caller_context_t *ct,
	int		flags)
{
	vnode_t		*vp;
	sfnode_t	*node;
	int		error;

	/*
	 * These should never happen
	 */
	ASSERT(name != NULL);
	ASSERT(strcmp(name, "..") != 0);

	error = sffs_lookup(dvp, name, &vp,
	    NULL, 0, NULL, cred, ct, NULL, NULL);
	if (error)
		return (error);
	node = VN2SFN(vp);

	mutex_enter(&sffs_lock);
	error = sfnode_access(VN2SFN(dvp), VEXEC | VWRITE, cred);
	if (error)
		goto done;

	/*
	 * If anything else is using this vnode, then fail the remove.
	 * Why?  Windows hosts can't sfprov_remove() a file that is open,
	 * so we have to sfprov_close() it first.
	 * There is no errno for this - since it's not a problem on UNIX,
	 * but ETXTBSY is the closest.
	 */
	if (node->sf_file != NULL) {
		if (vp->v_count > 1) {
			error = ETXTBSY;
			goto done;
		}
		(void)sfprov_close(node->sf_file);
		node->sf_file = NULL;
	}

	/*
	 * Remove the file on the host and mark the node as stale.
	 */
	error = sfprov_remove(node->sf_sffs->sf_handle, node->sf_path);
	if (error == ENOENT || error == 0)
		sfnode_make_stale(node);
done:
	mutex_exit(&sffs_lock);
	VN_RELE(vp);
	return (error);
}

/*ARGSUSED*/
static int
sffs_rename(
	vnode_t		*old_dir,
	char		*old_nm,
	vnode_t		*new_dir,
	char		*new_nm,
	cred_t		*cred,
	caller_context_t *ct,
	int		flags)
{
	char		*newpath;
	int		error;
	sfnode_t	*node;

	if (strcmp(new_nm, "") == 0 ||
	    strcmp(new_nm, ".") == 0 ||
	    strcmp(new_nm, "..") == 0 ||
	    strcmp(old_nm, "") == 0 ||
	    strcmp(old_nm, ".") == 0 ||
	    strcmp(old_nm, "..") == 0)
		return (EINVAL);

	/*
	 * make sure we have permission to do the rename
	 */
	mutex_enter(&sffs_lock);
	error = sfnode_access(VN2SFN(old_dir), VEXEC | VWRITE, cred);
	if (error == 0 && new_dir != old_dir)
		error = sfnode_access(VN2SFN(new_dir), VEXEC | VWRITE, cred);
	if (error)
		goto done;

	node = sfnode_lookup(VN2SFN(old_dir), old_nm, VNON);
	if (node == NULL) {
		error = ENOENT;
		goto done;
	}


	/*
	 * Rename the file on the host and in our caches.
	 */
	newpath = sfnode_construct_path(VN2SFN(new_dir), new_nm);
	error = sfprov_rename(node->sf_sffs->sf_handle, node->sf_path, newpath,
	    node->sf_type == VDIR);
	if (error == 0)
		sfnode_rename(node, VN2SFN(new_dir), newpath);
	else {
		kmem_free(newpath, strlen(newpath) + 1);
		if (error == ENOENT)
			sfnode_make_stale(node);
	}
done:
	mutex_exit(&sffs_lock);
	return (error);
}


/*ARGSUSED*/
static int
sffs_fsync(vnode_t *vp, int flag, cred_t *cr, caller_context_t *ct)
{
#if 0
	sfnode_t *node;

	/*
	 * Ask the host to sync any data it may have cached for open files.
	 * I don't think we care about errors.
	 */
	mutex_enter(&sffs_lock);
	node = VN2SFN(vp);
	if (node->sf_file != NULL)
		(void) sfprov_fsync(node->sf_file);
	mutex_exit(&sffs_lock);
#endif
	return (0);
}

/*
 * This may be the last reference, possibly time to close the file and
 * destroy the vnode. If the sfnode is stale, we'll destroy that too.
 */
/*ARGSUSED*/
static void
sffs_inactive(vnode_t *vp, cred_t *cr, caller_context_t *ct)
{
	sfnode_t *node;

	/*
	 * nothing to do if this isn't the last use
	 */
	mutex_enter(&sffs_lock);
	node = VN2SFN(vp);
	mutex_enter(&vp->v_lock);
	if (vp->v_count > 1) {
		--vp->v_count;
		mutex_exit(&vp->v_lock);
		mutex_exit(&sffs_lock);
		return;
	}

	/*
	 * There should never be cached data, since we don't support mmap().
	 */
	if (vn_has_cached_data(vp))
		panic("sffs_inactive() found cached data");

	/*
	 * destroy the vnode
	 */
	node->sf_vnode = NULL;
	mutex_exit(&vp->v_lock);
	vn_invalid(vp);
	vn_free(vp);
    LogFlowFunc(("  %s vnode cleared\n", node->sf_path));

	/*
	 * Close the sf_file for the node.
	 */
	if (node->sf_file != NULL) {
		(void)sfprov_close(node->sf_file);
		node->sf_file = NULL;
	}

	/*
	 * If the node is stale, we can also destroy it.
	 */
	if (node->sf_is_stale && node->sf_children == 0)
		sfnode_destroy(node);

	mutex_exit(&sffs_lock);
	return;
}

/*
 * All the work for this is really done in lookup.
 */
/*ARGSUSED*/
static int
sffs_open(vnode_t **vpp, int flag, cred_t *cr, caller_context_t *ct)
{
	return (0);
}

/*
 * All the work for this is really done in inactive.
 */
/*ARGSUSED*/
static int
sffs_close(
	vnode_t *vp,
	int flag,
	int count,
	offset_t offset,
	cred_t *cr,
	caller_context_t *ct)
{
	return (0);
}

/* ARGSUSED */
static int
sffs_seek(vnode_t *v, offset_t o, offset_t *no, caller_context_t *ct)
{
	if (*no < 0 || *no > MAXOFFSET_T)
		return (EINVAL);
	return (0);
}



/*
 * By returning an error for this, we prevent anything in sffs from
 * being re-exported by NFS
 */
/* ARGSUSED */
static int
sffs_fid(vnode_t *vp, fid_t *fidp, caller_context_t *ct)
{
	return (ENOTSUP);
}

/*
 * vnode operations for regular files
 */
const fs_operation_def_t sffs_ops_template[] = {
#if defined(VBOX_VFS_SOLARIS_10U6)
	VOPNAME_ACCESS,		sffs_access,
	VOPNAME_CLOSE,		sffs_close,
	VOPNAME_CREATE,		sffs_create,
	VOPNAME_FID,		sffs_fid,
	VOPNAME_FSYNC,		sffs_fsync,
	VOPNAME_GETATTR,	sffs_getattr,
	VOPNAME_INACTIVE,	sffs_inactive,
	VOPNAME_LOOKUP,		sffs_lookup,
	VOPNAME_MKDIR,		sffs_mkdir,
	VOPNAME_OPEN,		sffs_open,
	VOPNAME_PATHCONF,	sffs_pathconf,
	VOPNAME_READ,		sffs_read,
	VOPNAME_READDIR,	sffs_readdir,
	VOPNAME_REMOVE,		sffs_remove,
	VOPNAME_RENAME,		sffs_rename,
	VOPNAME_RMDIR,		sffs_rmdir,
	VOPNAME_SEEK,		sffs_seek,
	VOPNAME_WRITE,		sffs_write,
	NULL,			NULL
#else
	VOPNAME_ACCESS,		{ .vop_access = sffs_access },
	VOPNAME_CLOSE,		{ .vop_close = sffs_close },
	VOPNAME_CREATE,		{ .vop_create = sffs_create },
	VOPNAME_FID,		{ .vop_fid = sffs_fid },
	VOPNAME_FSYNC,		{ .vop_fsync = sffs_fsync },
	VOPNAME_GETATTR,	{ .vop_getattr = sffs_getattr },
	VOPNAME_INACTIVE,	{ .vop_inactive = sffs_inactive },
	VOPNAME_LOOKUP,		{ .vop_lookup = sffs_lookup },
	VOPNAME_MKDIR,		{ .vop_mkdir = sffs_mkdir },
	VOPNAME_OPEN,		{ .vop_open = sffs_open },
	VOPNAME_PATHCONF,	{ .vop_pathconf = sffs_pathconf },
	VOPNAME_READ,		{ .vop_read = sffs_read },
	VOPNAME_READDIR,	{ .vop_readdir = sffs_readdir },
	VOPNAME_REMOVE,		{ .vop_remove = sffs_remove },
	VOPNAME_RENAME,		{ .vop_rename = sffs_rename },
	VOPNAME_RMDIR,		{ .vop_rmdir = sffs_rmdir },
	VOPNAME_SEEK,		{ .vop_seek = sffs_seek },
	VOPNAME_WRITE,		{ .vop_write = sffs_write },
	NULL,			NULL
#endif
};

/*
 * Also, init and fini functions...
 */
int
sffs_vnode_init(void)
{
	int err;

	err = vn_make_ops("sffs", sffs_ops_template, &sffs_ops);
	if (err)
		return (err);

	avl_create(&sfnodes, sfnode_compare, sizeof (sfnode_t),
	    offsetof(sfnode_t, sf_linkage));
	avl_create(&stale_sfnodes, sfnode_compare, sizeof (sfnode_t),
	    offsetof(sfnode_t, sf_linkage));

	sffs_buffer = kmem_alloc(PAGESIZE, KM_SLEEP);

	return (0);
}

void
sffs_vnode_fini(void)
{
	if (sffs_ops)
		vn_freevnodeops(sffs_ops);
	ASSERT(avl_first(&sfnodes) == NULL);
	avl_destroy(&sfnodes);
	if (sffs_buffer != NULL) {
		kmem_free(sffs_buffer, PAGESIZE);
		sffs_buffer = NULL;
	}
}

/*
 * Utility at unmount to get all nodes in that mounted filesystem removed.
 */
int
sffs_purge(struct sffs_data *sffs)
{
	sfnode_t *node;
	sfnode_t *prev;

	/*
	 * Check that no vnodes are active.
	 */
	if (sffs->sf_rootnode->v_count > 1)
		return (-1);
	for (node = avl_first(&sfnodes); node;
	    node = AVL_NEXT(&sfnodes, node)) {
		if (node->sf_sffs == sffs && node->sf_vnode &&
		    node->sf_vnode != sffs->sf_rootnode)
			return (-1);
	}
	for (node = avl_first(&stale_sfnodes); node;
	    node = AVL_NEXT(&stale_sfnodes, node)) {
		if (node->sf_sffs == sffs && node->sf_vnode &&
		    node->sf_vnode != sffs->sf_rootnode)
			return (-1);
	}

	/*
	 * All clear to destroy all node information. Since there are no
	 * vnodes, the make stale will cause deletion.
	 */
	VN_RELE(sffs->sf_rootnode);
	mutex_enter(&sffs_lock);
	for (prev = NULL;;) {
		if (prev == NULL)
			node = avl_first(&sfnodes);
		else
			node = AVL_NEXT(&sfnodes, prev);

		if (node == NULL)
			break;

		if (node->sf_sffs == sffs) {
			if (node->sf_vnode != NULL)
				panic("vboxfs: purge hit active vnode");
			sfnode_make_stale(node);
		} else {
			prev = node;
		}
	}
done:
	mutex_exit(&sffs_lock);
	return (0);
}

static void
sfnode_print(sfnode_t *node)
{
	Log(("0x%p", node));
	Log((" type=%s (%d)",
	    node->sf_type == VDIR ? "VDIR" :
	    node->sf_type == VNON ? "VNON" :
	    node->sf_type == VREG ? "VREG" : "other", node->sf_type));
	Log((" ino=%d", (uint_t)node->sf_ino));
	Log((" path=%s", node->sf_path));
	Log((" parent=0x%p", node->sf_parent));
	if (node->sf_children)
		Log((" children=%d", node->sf_children));
	if (node->sf_vnode)
		Log((" vnode=0x%p", node->sf_vnode));
	Log(("%s\n", node->sf_is_stale ? " STALE" : ""));
}

void
sfnode_list()
{
	sfnode_t *n;
	for (n = avl_first(&sfnodes); n != NULL; n = AVL_NEXT(&sfnodes, n))
		sfnode_print(n);
	for (n = avl_first(&stale_sfnodes); n != NULL;
	    n = AVL_NEXT(&stale_sfnodes, n))
		sfnode_print(n);
}
