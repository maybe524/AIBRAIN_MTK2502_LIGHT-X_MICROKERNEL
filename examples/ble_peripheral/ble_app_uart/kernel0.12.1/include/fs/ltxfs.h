#pragma once

#include "include/fs.h"

#define LIGH_SUPER_BLK_SIZE 1024
#define LXTFS_NAME_LEN 20
#define LXT_N_BLOCKS 12

/*
 * Structure of a blocks group descriptor
 */
struct ligh_group_desc {
	__le32	bg_block_bitmap;		/* Blocks bitmap block */
	__le32	bg_inode_bitmap;		/* Inodes bitmap block */
	__le32	bg_inode_table;		/* Inodes table block */
	__le16	bg_free_blocks_count;	/* Free blocks count */
	__le16	bg_free_inodes_count;	/* Free inodes count */
	__le16	bg_used_dirs_count;	/* Directories count */
	__le16	bg_pad;
	__le32	bg_reserved[3];
};

/*
 * Constants relative to the data blocks
 */
#define	LIGH_NDIR_BLOCKS		12
#define	LIGH_IND_BLOCK			LIGH_NDIR_BLOCKS
#define	LIGH_DIND_BLOCK			(LIGH_IND_BLOCK + 1)
#define	LIGH_TIND_BLOCK			(LIGH_DIND_BLOCK + 1)
#define	LIGH_N_BLOCKS			(LIGH_TIND_BLOCK + 1)

#define LXTFLG_IN_L0			(1 << 0)
#define LXTFLG_IN_L1			(1 << 1)

/*
 * Structure of an inode on the disk
 */
struct ltx_inode {
	__le16	i_mode;		/* File mode */
	__le16	i_uid;		/* Low 16 bits of Owner Uid */
	__le32	i_size;		/* Size in bytes */
	__le32	i_atime;	/* Access time */
	__le32	i_ctime;	/* Creation time */
	__le32	i_mtime;	/* Modification time */
	__le32	i_dtime;	/* Deletion Time */
	//__le16	i_gid;		/* Low 16 bits of Group Id */
	//__le16	i_links_count;	/* Links count */
	__le16	i_blocks;	/* Blocks count */
	__le32	i_flags;	/* File flags */
	//union {
	//	struct {
	//		__le32  l_i_reserved1;
	//	} linux1;
	//	struct {
	//		__le32  h_i_translator;
	//	} hurd1;
	//	struct {
	//		__le32  m_i_reserved1;
	//	} masix1;
	//} osd1;				/* OS dependent 1 */
	__u8	i_block[LIGH_N_BLOCKS];/* Pointers to blocks */
	__le16	i_parent, i_child;
	//__le32	i_generation;	/* File version (for NFS) */
	//__le32	i_file_acl;	/* File ACL */
	//__le32	i_dir_acl;	/* Directory ACL */
	//__le32	i_faddr;	/* Fragment address */
	//union {
	//	struct {
	//		__u8	l_i_frag;	/* Fragment number */
	//		__u8	l_i_fsize;	/* Fragment size */
	//		__u16	i_pad1;
	//		__le16	l_i_uid_high;	/* these 2 fields    */
	//		__le16	l_i_gid_high;	/* were reserved2[0] */
	//		__u32	l_i_reserved2;
	//	} linux2;
	//	struct {
	//		__u8	h_i_frag;	/* Fragment number */
	//		__u8	h_i_fsize;	/* Fragment size */
	//		__le16	h_i_mode_high;
	//		__le16	h_i_uid_high;
	//		__le16	h_i_gid_high;
	//		__le32	h_i_author;
	//	} hurd2;
	//	struct {
	//		__u8	m_i_frag;	/* Fragment number */
	//		__u8	m_i_fsize;	/* Fragment size */
	//		__u16	m_pad1;
	//		__u32	m_i_reserved2[2];
	//	} masix2;
	//} osd2;				/* OS dependent 2 */
};

/*
 * Structure of the super block
 */
struct ltx_super_block {
	__le32	s_super_block_addr;

	__le16	s_inode_bitmap_addr;
	__le16	s_block_bitmap_addr;
	__le16	s_dentry_bitmap_addr;
	__le16	s_sysdb_bitmap_addr;

	__le16	s_root_inode_addr;
	__le32	s_root_block_addr;
	__le16	s_root_direntry_addr;
	__le16	s_root_sysdb_addr;

	__le16	s_inodes_count;		/* Inodes count */
	__le16	s_blocks_count;		/* Blocks count */
	__le16	s_dentry_count;
	__le16	s_sysdb_count;

	__le16	s_free_blocks_count;	/* Free blocks count */
	__le16	s_free_inodes_count;	/* Free inodes count */
	__le16	s_free_dentry_count;
	__le16	s_free_sysdb_count;

	__le32	s_mtime;		/* Mount time */
	__le32	s_wtime;		/* Write time */

	__le32	s_magic;		/* Magic signature */

	__le32	s_first_ino; 		/* First non-reserved inode */
	__le16  s_inode_size; 		/* size of inode structure */
	__le16	s_page_size;
	__le16	s_sblock_size;
	__le16	s_dentry_size;
	__le16	s_sysdb_size;

};

/*
 * Structure of a directory entry
 */
#define LIGH_NAME_LEN 255

struct ltx_direntry {
	__u8 inode, parent;			/* Inode number */
	__u8 rec_len;		/* Directory entry length */
	__u8 name_len;		/* Name length */
	__le16 file_type;
	char name[LXTFS_NAME_LEN];	/* File name */
};

/*
 * Ext2 directory file types.  Only the low 3 bits are used.  The
 * other bits are reserved for now.
 */
enum {
	LIGH_FT_UNKNOWN		= 0,
	LIGH_FT_REG_FILE	= 1,
	LIGH_FT_DIR			= 2,
	LIGH_FT_CHRDEV		= 3,
	LIGH_FT_BLKDEV		= 4,
	LIGH_FT_FIFO		= 5,
	LIGH_FT_SOCK		= 6,
	LIGH_FT_SYMLINK		= 7,
	LIGH_FT_MAX
};

struct ltx_inode_info {
	struct inode vfs_inode;
	__le32	i_data[LXT_N_BLOCKS];
	struct ltx_inode *i_lxin;
};

struct ltx_sb_info {
	struct ltx_super_block lx_sb;
	//struct ltx_group_desc *gdt;
};


/*
 * Structure of a blocks group descriptor
 */
struct ltx_group_desc {
	__le32	bg_block_bitmap;		/* Blocks bitmap block */
	__le32	bg_inode_bitmap;		/* Inodes bitmap block */
	__le32	bg_inode_table;		/* Inodes table block */
	__le16	bg_free_blocks_count;	/* Free blocks count */
	__le16	bg_free_inodes_count;	/* Free inodes count */
	__le16	bg_used_dirs_count;	/* Directories count */
	__le16	bg_pad;
	__le32	bg_reserved[3];
};

/*
 * Constants relative to the data blocks
 */
#define	LTX_NDIR_BLOCKS		12
#define	LTX_IND_BLOCK			LTX_NDIR_BLOCKS
#define	LTX_DIND_BLOCK			(LTX_IND_BLOCK + 1)
#define	LTX_TIND_BLOCK			(LTX_DIND_BLOCK + 1)
#define	LTX_N_BLOCKS			(LTX_TIND_BLOCK + 1)

/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ltx_direntry_2 {
	__le32	inode;			/* Inode number */
	__le16	rec_len;		/* Directory entry length */
	__u8	name_len;		/* Name length */
	__u8	file_type;
	char	name[LIGH_NAME_LEN];	/* File name */
};

#define LXT_ERASE (1 << 1)
#define LXT_WRITE	 (1 << 2)

struct lxtfs_map {
	struct ltx_super_block lxsb;
	__u32 *lxin_bitmap;
	__u32 *lxbl_bitmap;
	__u32 *lxde_bitmap;
	struct ltx_inode lxin_table[32];
	struct ltx_direntry_2 lxde_table[32];
};

/*
 *  This Data struct is used for data base struct
 */
struct ltx_qstr {
	char id;
	char *name;
	unsigned int nlen;
	char *val;
	unsigned int vlen;
};


#define DBASE_NAME_LEN	20
#define DBASE_VAL_LEN	25
struct ltx_dbase {
	__u8 id;
	__u8 name_len;
	__u8 name[DBASE_NAME_LEN];
	__u8 val_len;
	__u8 val[DBASE_VAL_LEN];
};

#define LTXFL_CLEAR	(1 << 0)

static inline struct ltx_inode_info *LXT_I(struct inode *inode)
{
	return container_of(inode, struct ltx_inode_info, vfs_inode);
}

extern struct task_struct *current;

#define LXTFS_MAGIC_NUMBER	('lxfs')	// 0x6C786673

static int ltx_lookup(struct inode *parent, struct dentry *dentry, struct nameidata *nd);
static int ltx_open(struct file *fp, struct inode *inode);
static int ltx_close(struct file *fp);
static ssize_t ltx_read(struct file *fp, void *buff, __size_t size, loff_t *off);
static ssize_t ltx_write(struct file *fp, const void *buff, __size_t size, loff_t *off);
static int ltx_mkdir(struct inode *inode, struct dentry *dentry, umode_t mode);
static int ltx_readdir(struct file *fp, void *dirent, filldir_t filldir);
static int ltx_opendir(struct file *fp, struct inode *inode);
static int ltx_reg_create(struct inode *parent, struct dentry *dentry, umode_t mode, struct nameidata *nd);
static struct ltx_inode *ltx_new_inode(struct super_block *sb, char *i_bitmap, short *id);
static struct ltx_direntry *ltx_new_dentry(struct super_block *sb, char *d_bitmap, short *id);
static char *ltx_new_block(struct super_block *sb, char *b_bitmap, short *id);
static void ltx_d_install(struct ltx_direntry *lxde, struct dentry *dentry, __u16 file_type);
static struct inode *ltx_alloc_inode(struct super_block *sb);
static void ltx_read_inode(struct inode *inode);
static int ltx_rmdir(struct inode *, struct dentry *);
static ssize_t ltx_db_read(struct file *fp, void *buff, __size_t size, loff_t *off);
static ssize_t ltx_db_write(struct file *fp, const void *buff, __size_t size, loff_t *off);
static int ltx_db_open(struct file *fp, struct inode *inode);
static int ltx_db_close(struct file *fp);
static int ltx_db_rm(struct inode *inode, struct dentry *dentry);