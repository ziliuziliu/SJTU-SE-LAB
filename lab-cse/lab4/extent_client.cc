// RPC stubs for clients to talk to extent_server

#include "rpc.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

int extent_client::last_port = 0;

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }

  srand(time(NULL)^last_port);
  rextent_port = ((rand()%32000) | (0x1 << 10)) + 427;
  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rextent_port;
  id = host.str();
  last_port = rextent_port;
  rpcs *rlsrpc = new rpcs(rextent_port);
  rlsrpc->reg(rextent_protocol::refresh, this, &extent_client::refresh_handler);
  rlsrpc->reg(rextent_protocol::require, this, &extent_client::require_handler);

  file_cache[1] = new file_item(true, true, extent_protocol::T_DIR);    
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::create, id, type, eid);
  file_cache[eid] = new file_item(false, false, type);
  return ret;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  if (!file_cache.count(eid)) {
    ret = cl->call(extent_protocol::get, id, eid, buf);
    file_cache[eid] = new file_item(false, true, 0);
    file_cache[eid]->buf.assign(buf);
    file_cache[eid]->attr.size = buf.size();
  }
  else if (file_cache[eid]->buf_dirty) {
    ret = cl->call(extent_protocol::get, id, eid, buf);
    file_cache[eid]->buf.assign(buf);
    file_cache[eid]->attr.size = buf.size();
    file_cache[eid]->buf_dirty = false;
  }
  else buf.assign(file_cache[eid]->buf);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  if (!file_cache.count(eid)) {
    ret = cl->call(extent_protocol::getattr, id, eid, attr);
    file_cache[eid] = new file_item(true, false, attr.type);
    file_cache[eid]->attr = attr;
  }
  else if (file_cache[eid]->attr_dirty) {
    ret = cl->call(extent_protocol::getattr, id, eid, attr);
    file_cache[eid]->attr = attr;
    file_cache[eid]->attr_dirty = false;
  }
  else attr = file_cache[eid]->attr;
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  if (!file_cache[eid]->keep_writing) {
    ret = cl->call(extent_protocol::put, id, eid, buf, r);
    file_cache[eid]->keep_writing = true;
  }
  if (file_cache.count(eid)) {
    file_cache[eid]->buf.assign(buf);
    file_cache[eid]->attr.size = buf.size();
    file_cache[eid]->attr.atime = file_cache[eid]->attr.mtime = file_cache[eid]->attr.ctime = time(NULL);
    file_cache[eid]->buf_dirty = file_cache[eid]->attr_dirty = false;
  }
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  ret = cl->call(extent_protocol::remove, id, eid, r);
  file_cache.erase(eid);
  return ret;
}

rextent_protocol::status
extent_client::refresh_handler(extent_protocol::extentid_t eid, int &) {
  if (file_cache.count(eid)) {
    file_cache[eid]->attr_dirty = file_cache[eid]->buf_dirty = true;
    file_cache[eid]->keep_writing = false;
  }
  return rextent_protocol::OK;
}

rextent_protocol::status
extent_client::require_handler(extent_protocol::extentid_t eid, int &) {
  int r;
  extent_protocol::status ret = cl->call(extent_protocol::put, id, eid, file_cache[eid]->buf, r);
  if (file_cache.count(eid)) {
    file_cache[eid]->attr.atime = file_cache[eid]->attr.mtime = file_cache[eid]->attr.ctime = time(NULL);
    file_cache[eid]->buf_dirty = file_cache[eid]->attr_dirty = false;
  }
  return rextent_protocol::OK;
}