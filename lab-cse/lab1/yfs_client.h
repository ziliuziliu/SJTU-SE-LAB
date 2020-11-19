#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <list>
#include <map>

class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  enum filetype { FILE=1, DIR, SYMLINK };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct symlinkinfo {
      unsigned long long size;
      unsigned long atime;
      unsigned long mtime;
      unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
  std::map<std::string, yfs_client::inum> dir_pair_map[10050];
  filetype file_type_map[10050];
  fileinfo file_attr_map[10050];
  dirinfo dir_attr_map[10050];
  symlinkinfo symlink_attr_map[10050];

 public:
  yfs_client();
  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);
  bool issymlink(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);
  int getsymlink(inum, symlinkinfo &);

  int addtoparent(inum, const char *, inum);
  int setattr(inum, size_t);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &);
  int readdir(inum, std::list<dirent> &);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
  int mkdir(inum , const char *, mode_t , inum &);
  int symlink(const char *, inum, const char *, inum &);
  int mksym(inum, const char *, mode_t, inum &);
  int readlink(inum, std::string &);
  
  /** you may need to add symbolic link related methods here.*/
};

#endif 
