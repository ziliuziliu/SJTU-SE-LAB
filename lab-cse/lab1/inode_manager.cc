#include "inode_manager.h"
#include <ctime>
#include<linux/types.h>

__u64 rdtsc()
{
        __u32 lo,hi;

        __asm__ __volatile__
        (
         "rdtsc":"=a"(lo),"=d"(hi)
        );
        return (__u64)hi<<32|lo;
}
// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf)
{
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
  uint32_t id = head->id;
  block_node *tmp = head;
  head = head->next;
  delete tmp;
  //printf("ALLOC BLOCK %d\n", id);
  return id;
  //char buf[BLOCK_SIZE];
  //uint32_t byte_offset, bit_offset;
  //for (uint32_t i=START_OF_DATA_BLOCK;i<sb.nblocks;i++) {
    //if (using_blocks[i] == 1) continue;
    //using_blocks[i]=1;
    //read_block(BBLOCK(i), buf);
    //byte_offset = (i%BPB)/8;
    //bit_offset = (i%BPB)%8;
    //buf[byte_offset] |= (1<<bit_offset);
    //write_block(BBLOCK(i), buf);
    //return i;
  //}
  //printf("\tbm: error! not enough block to alloc\n");
  //return 0;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  //printf("FREE BLOCK %d\n", id);
  if (id) {
    tail->next = new block_node(id);
    tail = tail->next;
  }
  //char buf[BLOCK_SIZE];
  //uint32_t byte_offset, bit_offset;
  //using_blocks[id] = 0;
  //read_block(BBLOCK(id), buf);
  //byte_offset = (id%BPB)/8;
  //bit_offset = (id%BPB)%8;
  //buf[byte_offset] ^= (1<<bit_offset);
  //write_block(BBLOCK(id), buf);
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

  block_node *prev = NULL;
  for (uint32_t i=START_OF_DATA_BLOCK;i<sb.nblocks;i++) {
    block_node *tmp = new block_node(i);
    if (i != START_OF_DATA_BLOCK) prev->next = tmp;
    else head = tmp;
    prev = tmp;	  
  } 
  tail = prev;
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
  //inode_head = 0;
  bm = new block_manager();
  inode_node *prev = NULL;
  for (int i=1;i<INODE_NUM;i++) {
    inode_node *tmp = new inode_node(i);
    if (i != 1) prev->next = tmp;
    else head = tmp;
    prev = tmp;
  }
  tail = prev;
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    //printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
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
  //for (int i=1;i<INODE_NUM;i++) {
    // inode *ino = get_inode(i);
    //if (!inode_map[i]) {
      //inode_map[i]=1;
      int inum = head->inum;
      head = head->next;
      //unsigned int cur_time = (unsigned)std::time(0);
      inode *ino = new inode();
      //inode *ino = inode_pool + inode_head; 
      ino->type = type;
      ino->size = 0;
      ino->atime = ino->ctime = ino->mtime = (unsigned)std::time(0);
      //inode_head++;
      put_inode(inum, ino);
      return inum;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  inode *ino = get_inode(inum);
  if (!ino || !(ino->type)) return;
  ino->type = 0;
  //inode_map[inum]=0;
  put_inode(inum, ino);
  tail->next = new inode_node(inum);
  tail = tail->next;
  //free(ino);
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
    //printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    //printf("\tim: inode not exist\n");
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
  inode *ino = get_inode(inum);
  char *rv = (char *)malloc(200050);
  int block_cnt=0, size_left;
  *size = size_left = ino->size;
  for (int i=0;i<=32;i++) {
    if (i!=32) {
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
  //__u64 begin,end;
  //begin = rdtsc();
  //printf("Write file to %d size %d\n", inum, size);
  inode *ino = get_inode(inum);

  // ifree old block
  int size_left = size;
  /*for (int i=0;i<=32;i++) {
    if (i!=32) {
      bm->free_block(ino->blocks[i]);
      old_size -= BLOCK_SIZE;
      if (old_size <= 0) break;
    }
    else {
      unsigned char buf2[BLOCK_SIZE];
      bm->read_block(ino->blocks[i], (char *)buf2);
      for (int j=0;j<BLOCK_SIZE;j+=4) {
        blockid_t block_id = *(blockid_t*) (buf2+j);
        //block_id |= buf2[j]; block_id |= (buf2[j+1]<<8);
        //block_id |= (buf2[j+2]<<16); block_id |= (buf2[j+3]<<24);
        bm->free_block(block_id);
        old_size -= BLOCK_SIZE;
        if (old_size <= 0) break;
      }
    }
  }

  //allocate and write new block
  int block_cnt = 0;
  ino->size = size;
  for (int i=0;i<=32;i++) {
    if (i!=32) {
      ino->blocks[i] = bm->alloc_block();
      bm->write_block(ino->blocks[i],buf+block_cnt*BLOCK_SIZE);
      block_cnt++;
      size_left -= BLOCK_SIZE;
      if (size_left <= 0) break;
    }
    else {
      unsigned char block_nums[BLOCK_SIZE];
      ino->blocks[i] = bm->alloc_block();
      for (int j=0;j<BLOCK_SIZE;j+=4) {
        blockid_t block_id = bm->alloc_block();
        bm->write_block(block_id, buf+block_cnt*BLOCK_SIZE);
        //for (int k=j;k<j+4;k++) {
        //  block_nums[k] = block_id&(0xff);
        //  block_id >>= 8;
        //}
        *(blockid_t *)(block_nums+j) = block_id;
        block_cnt++;
        size_left -= BLOCK_SIZE;
        if (size_left <= 0) break;
      }
      bm->write_block(ino->blocks[i],(char *) block_nums);
    }
  }*/

  int block_cnt = 0, block_cnt_prev = ino->block_cnt;
  ino->size = size;
  for (int i=0;i<=32;i++) {
    if (i!=32) {
	if (block_cnt >= block_cnt_prev) ino->blocks[i] = bm->alloc_block();
	bm->write_block(ino->blocks[i], buf+block_cnt*BLOCK_SIZE);
	block_cnt++;
	size_left -= BLOCK_SIZE;
	if (size_left <= 0) break;
    }
    else {
       unsigned char block_nums[BLOCK_SIZE];
       if (block_cnt_prev <= 32) ino->blocks[i] = bm->alloc_block();
       for (int j=0;j<BLOCK_SIZE;j+=4) {
         blockid_t block_id;
         //if (block_cnt >= block_cnt_prev) 
	   block_id = bm->alloc_block();
         //TODO  
         bm->write_block(block_id, buf+block_cnt*BLOCK_SIZE);
         *(blockid_t *)(block_nums+j) = block_id;
         block_cnt++;
         size_left -= BLOCK_SIZE;
         if (size_left <= 0) break;
      }
      bm->write_block(ino->blocks[i],(char *) block_nums);
    }
  }
  if (block_cnt_prev > block_cnt) {
     for (int i=block_cnt_prev-1;i>=block_cnt;i--)
       bm->free_block(ino->blocks[i]);
  }
  ino->block_cnt = block_cnt; 

  //update inode info
  ino->atime = ino->ctime = ino->mtime = (unsigned)std::time(0);
  put_inode(inum, ino);
  free(ino);
  //end=rdtsc();
  //printf("%d blocks\n", block_cnt);
  //printf("%llu cycles\n", end-begin);
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
  inode *ino = get_inode(inum);
  if (!ino) return;
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
  inode *ino = get_inode(inum);
  int old_size = ino->size;
  for (int i=0;i<=32;i++) {
    if (i!=32) {
      bm->free_block(ino->blocks[i]);
      old_size -= BLOCK_SIZE;
      if (old_size <= 0) break;
    }
    else {
      unsigned char buf2[BLOCK_SIZE];
      bm->read_block(ino->blocks[i], (char *)buf2);
      for (int j=0;j<BLOCK_SIZE;j+=4) {
        blockid_t block_id = *(blockid_t*) (buf2+j);
        //block_id |= buf2[j]; block_id |= (buf2[j+1]<<8);
        //block_id |= (buf2[j+2]<<16); block_id |= (buf2[j+3]<<24);
        bm->free_block(block_id);
        old_size -= BLOCK_SIZE;
        if (old_size <= 0) break;
      }
    }
  }
  free_inode(inum);
  return;
}
