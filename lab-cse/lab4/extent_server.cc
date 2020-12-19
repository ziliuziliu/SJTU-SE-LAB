// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "handle.h"

extent_server::extent_server() 
{
  im = new inode_manager();

  client_map[1] = new client_info();
}

int extent_server::create(std::string cl, uint32_t type, extent_protocol::extentid_t &id)
{
  // alloc a new inode and return inum
 // printf("extent_server: create inode\n");
  id = im->alloc_inode(type);

  client_map[id] = new client_info();
  client_map[id]->clients.insert(cl);
  return extent_protocol::OK;
}

int extent_server::put(std::string cl, extent_protocol::extentid_t id, std::string buf, int &)
{
  id &= 0x7fffffff;
  
  const char * cbuf = buf.c_str();
  int size = buf.size();
  im->write_file(id, cbuf, size);

  client_map[id]->clients.insert(cl);
  refresh_client(id, cl);
  return extent_protocol::OK;
}

int extent_server::get(std::string cl, extent_protocol::extentid_t id, std::string &buf)
{
  // printf("extent_server: get %lld\n", id);

  id &= 0x7fffffff;

  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    free(cbuf);
  }
  
  client_map[id]->clients.insert(cl);
  return extent_protocol::OK;
}

int extent_server::getattr(std::string cl, extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  // printf("extent_server: getattr %lld\n", id);

  id &= 0x7fffffff;
  
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->getattr(id, attr);
  a = attr;

  client_map[id]->clients.insert(cl);
  return extent_protocol::OK;
}

int extent_server::remove(std::string cl, extent_protocol::extentid_t id, int &)
{
  // printf("extent_server: write %lld\n", id);

  id &= 0x7fffffff;
  im->remove_file(id);
 
  client_map[id]->clients.insert(cl);
  refresh_client(id, cl);
  return extent_protocol::OK;
}

void extent_server::refresh_client(extent_protocol::extentid_t eid, std::string cl) {
  std::set<std::string>::iterator it;
  for (it=client_map[eid]->clients.begin(); it!=client_map[eid]->clients.end(); it++) {
    if (*it == cl) continue;
    int r;
    handle(*it).safebind()->call(rextent_protocol::refresh, eid, r);
  }
}