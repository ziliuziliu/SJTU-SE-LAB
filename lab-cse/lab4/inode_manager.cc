#include "inode_manager.h"
#include <ctime>
// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;
  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;
  memcpy(blocks[id], buf, BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  char buf[BLOCK_SIZE];
  uint32_t byte_offset, bit_offset;
  for (uint32_t i=START_OF_DATA_BLOCK;i<sb.nblocks;i++) {
    if (using_blocks.count(i)>0) continue;
    using_blocks[i]=1;
    read_block(BBLOCK(i), buf);
    byte_offset = (i%BPB)/8;
    bit_offset = (i%BPB)%8;
    buf[byte_offset] |= (1<<bit_offset);
    write_block(BBLOCK(i), buf);
    return i;
  }
  // printf("\tbm: error! not enough block to alloc\n");
  return 0;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  char buf[BLOCK_SIZE];
  uint32_t byte_offset, bit_offset;
  using_blocks.erase(id);
  read_block(BBLOCK(id), buf);
  byte_offset = (id%BPB)/8;
  bit_offset = (id%BPB)%8;
  buf[byte_offset] ^= (1<<bit_offset);
  write_block(BBLOCK(id), buf);
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  for (int i=1;i<INODE_NUM;i++) inode_map[i] = 0;
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    // printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  for (int i=1;i<INODE_NUM;i++) {
    if (!inode_map[i]) {
      inode *ino = new inode();
      ino->type = type;
      ino->size = 0;
      ino->atime = ino->ctime = ino->mtime = (unsigned)std::time(0);
      put_inode(i, ino);
      inode_map[i] = 1;
      return i;
    }
  }
  return 0;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  // printf("\tim: free inode %d\n",inum);
  inode *ino = get_inode(inum);
  if (!ino || !(ino->type)) return;
  ino->type = 0;
  put_inode(inum, ino);
  free(ino);
  inode_map[inum] = 0;
  return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  if (inum < 0 || inum >= INODE_NUM) {
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
  // printf("\tim: reading file %d\n",inum);  
  inode *ino = get_inode(inum);
  char *rv = (char *)malloc(200050);
  int block_cnt=0, size_left;
  *size = size_left = ino->size;
  // printf("size: %d\n",ino->size);
  for (int i=0;i<=100;i++) {
    if (i!=100) {
      bm->read_block(ino->blocks[i], rv+block_cnt*BLOCK_SIZE);
      block_cnt++;
      size_left -= BLOCK_SIZE;
      if (size_left <= 0) break;
    }
    else {
      unsigned char buf[BLOCK_SIZE];
      bm->read_block(ino->blocks[i], (char *)buf);
      for (int j=0;j<BLOCK_SIZE;j+=4) {
        blockid_t block_id = 0;
        block_id |= buf[j]; block_id |= (buf[j+1]<<8);
        block_id |= (buf[j+2]<<16); block_id |= (buf[j+3]<<24);
        bm->read_block(block_id, rv+block_cnt*BLOCK_SIZE);
        block_cnt++;
        size_left -= BLOCK_SIZE;
        if (size_left <= 0) break;
      }
    }
  }
  ino->atime = (unsigned)std::time(0);
  put_inode(inum, ino);
  free(ino);
  *buf_out = rv;
  return;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
  // printf("\tim: writing file %d\n",inum);
  inode *ino = get_inode(inum);

  // free old block
  int old_size = ino->size, size_left = size;
  for (int i=0;i<=100;i++) {
    if (i!=100) {
      bm->free_block(ino->blocks[i]);
      old_size -= BLOCK_SIZE;
      if (old_size <= 0) break;
    }
    else {
      unsigned char buf2[BLOCK_SIZE];
      bm->read_block(ino->blocks[i], (char *)buf2);
      for (int j=0;j<BLOCK_SIZE;j+=4) {
        blockid_t block_id = 0;
        block_id |= buf2[j]; block_id |= (buf2[j+1]<<8);
        block_id |= (buf2[j+2]<<16); block_id |= (buf2[j+3]<<24);
        bm->free_block(block_id);
        old_size -= BLOCK_SIZE;
        if (old_size <= 0) break;
      }
    }
  }

  //allocate and write new block
  int block_cnt = 0;
  ino->size = size;
  for (int i=0;i<=100;i++) {
    // printf("i = %d\n", i);
    if (i!=100) {
      ino->blocks[i] = bm->alloc_block();
      if (size_left < BLOCK_SIZE) {
         char tail_block[BLOCK_SIZE];
	 memcpy(tail_block, buf+block_cnt*BLOCK_SIZE, size_left);
         bm->write_block(ino->blocks[i], tail_block);
      }
      else bm->write_block(ino->blocks[i],buf+block_cnt*BLOCK_SIZE);
      block_cnt++;
      size_left -= BLOCK_SIZE;
      if (size_left <= 0) break;
    }
    else {
      unsigned char block_nums[BLOCK_SIZE];
      ino->blocks[i] = bm->alloc_block();
      for (int j=0;j<BLOCK_SIZE;j+=4) {
        blockid_t block_id = bm->alloc_block();
        if (size_left < BLOCK_SIZE) {
            char tail_block[BLOCK_SIZE];
            memcpy(tail_block, buf+block_cnt*BLOCK_SIZE, size_left);
            bm->write_block(block_id, tail_block);
        }
        else bm->write_block(block_id, buf+block_cnt*BLOCK_SIZE);
        for (int k=j;k<j+4;k++) {
          block_nums[k] = block_id&(0xff);
          block_id >>= 8;
        }
        block_cnt++;
        size_left -= BLOCK_SIZE;
        if (size_left <= 0) break;
      }
      bm->write_block(ino->blocks[i],(char *) block_nums);
    }
  }

  //update inode info
  ino->atime = ino->ctime = ino->mtime = (unsigned)std::time(0);
  put_inode(inum, ino);
  free(ino);
  return;
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  // printf("\tim: getattr %d\n",inum);
  inode *ino = get_inode(inum);
  if (!ino) {
     a.type = 0;
     return;
  }
  a.type = ino->type;
  a.atime = ino->atime;
  a.ctime = ino->ctime;
  a.mtime = ino->mtime;
  a.size = ino->size;
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  // printf("\tim: removing file %d\n",inum);
  inode *ino = get_inode(inum);
  int old_size = ino->size;
  for (int i=0;i<=100;i++) {
    if (i!=100) {
      bm->free_block(ino->blocks[i]);
      old_size -= BLOCK_SIZE;
      if (old_size <= 0) break;
    }
    else {
      unsigned char buf2[BLOCK_SIZE];
      bm->read_block(ino->blocks[i], (char *)buf2);
      for (int j=0;j<BLOCK_SIZE;j+=4) {
        blockid_t block_id = 0;
        block_id |= buf2[j]; block_id |= (buf2[j+1]<<8);
        block_id |= (buf2[j+2]<<16); block_id |= (buf2[j+3]<<24);
        bm->free_block(block_id);
        old_size -= BLOCK_SIZE;
        if (old_size <= 0) break;
      }
    }
  }
  free_inode(inum);
  return;
}
