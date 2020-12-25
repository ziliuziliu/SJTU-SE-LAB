// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "extent_server.h"

struct file_item {
    bool buf_dirty, attr_dirty, keep_writing;
    std::string buf;
    extent_protocol::attr attr;
    file_item(bool buf_dirty, bool attr_dirty, uint32_t type)
        :buf_dirty(buf_dirty),attr_dirty(attr_dirty) {
        keep_writing = false;
        attr.type = type;
        attr.size = 0;
        attr.atime = attr.mtime = attr.ctime = time(NULL);
    } 
};

class extent_client {
 private:
  rpcc *cl;
  std::map<extent_protocol::extentid_t, file_item *> file_cache;
  int rextent_port;
  std::string id;

 public:
  static int last_port;
  extent_client(std::string dst);

  extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			                        std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr &attr);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);

  rextent_protocol::status refresh_handler(extent_protocol::extentid_t eid, int &);
  rextent_protocol::status require_handler(extent_protocol::extentid_t eid, int &);
};

#endif