// /fs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>

yfs_client::yfs_client()
{
    ec = new extent_client();
    file_type_map[1] = DIR;
    dirinfo din;
    din.atime = din.ctime = din.mtime = (unsigned)std::time(0);
    dir_attr_map[1] = din;
}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client();
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
//    extent_protocol::attr a;
//
//    if (ec->getattr(inum, a) != extent_protocol::OK) {
//        printf("error getting attr\n");
//        return false;
//    }
//
//    if (a.type == extent_protocol::T_FILE) {
//        printf("isfile: %lld is a file\n", inum);
//        return true;
//    }
//    printf("isfile: %lld is not a file\n", inum);
//    return false;
    if (file_type_map.count(inum) == 0) return false;
    if (file_type_map[inum] == FILE) return true;
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 *
 * */
bool
yfs_client::issymlink(inum inum)
{
//    extent_protocol::attr a;
//
//    if (ec->getattr(inum, a) != extent_protocol::OK) {
//        printf("error getting attr\n");
//        return false;
//    }
//
//    if (a.type == extent_protocol::T_SYMLINK) {
//        printf("isfile: %lld is a symlink\n", inum);
//        return true;
//    }
//    printf("isfile: %lld is not a symlink\n", inum);
//    return false;
    if (file_type_map.count(inum) == 0) return false;
    if (file_type_map[inum] == SYMLINK) return true;
    return false;
}

bool
yfs_client::isdir(inum inum)
{
//    extent_protocol::attr a;
//
//    if (ec->getattr(inum, a) != extent_protocol::OK) {
//        printf("error getting attr\n");
//        return false;
//    }
//
//    if (a.type == extent_protocol::T_DIR) {
//        printf("isfile: %lld is a dir\n", inum);
//        return true;
//    }
//    printf("isfile: %lld is not a dir\n", inum);
//    return false;
    if (file_type_map.count(inum) == 0) return false;
    if (file_type_map[inum] == DIR) return true;
    return false;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
//    int r = OK;
//
//    printf("getfile %016llx\n", inum);
//    extent_protocol::attr a;
//    if (ec->getattr(inum, a) != extent_protocol::OK) {
//        r = IOERR;
//        goto release;
//    }
//
//    fin.atime = a.atime;
//    fin.mtime = a.mtime;
//    fin.ctime = a.ctime;
//    fin.size = a.size;
//    printf("getfile %016llx -> sz %llu\n", inum, fin.size);
//
//release:
//    return r;
    int r = OK;
    fin = file_attr_map[inum];
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
//    int r = OK;
//
//    printf("getdir %016llx\n", inum);
//    extent_protocol::attr a;
//    if (ec->getattr(inum, a) != extent_protocol::OK) {
//        r = IOERR;
//        goto release;
//    }
//    din.atime = a.atime;
//    din.mtime = a.mtime;
//    din.ctime = a.ctime;
//
//release:
//    return r;
    int r = OK;
    din = dir_attr_map[inum];
    return r;
}

int
yfs_client::getsymlink(inum inum, symlinkinfo &sin)
{
//    int r = OK;
//
//    printf("getdir %016llx\n", inum);
//    extent_protocol::attr a;
//    if (ec->getattr(inum, a) != extent_protocol::OK) {
//        r = IOERR;
//        goto release;
//    }
//    sin.atime = a.atime;
//    sin.mtime = a.mtime;
//    sin.ctime = a.ctime;
//
//    release:
//    return r;
    int r = OK;
    sin = symlink_attr_map[inum];
    return r;
}

#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;
    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    std::string buf;

    ec->get(ino, buf);
    if (file_type_map[ino] == FILE) file_attr_map[ino].atime = (unsigned)std::time(0);
    else if (file_type_map[ino] == SYMLINK) symlink_attr_map[ino].atime = (unsigned)std::time(0);

    buf.resize(size);

    ec->put(ino, buf);
    if (file_type_map[ino] == FILE) {
        fileinfo fin;
        fin.mtime = fin.ctime = fin.atime = (unsigned)std::time(0);
        file_attr_map[ino] = fin;
    }
    else if (file_type_map[ino] == SYMLINK) {
        symlinkinfo sin;
        sin.mtime = sin.ctime = sin.atime = (unsigned) std::time(0);
        symlink_attr_map[ino] = sin;
    }
    return r;
}

int
yfs_client::addtoparent(inum parent, const char *name, inum child)
{
    std::string buf;

    ec->get(parent, buf);
    dir_attr_map[parent].atime = (unsigned)std::time(0);

//    buf.append(name);buf.append(",");
//    buf.append(filename(child));buf.append(":");
    ec->put(parent, buf);
    dirinfo din;
    din.atime = din.mtime = din.ctime = (unsigned) std::time(0);

    std::map<std::string, yfs_client::inum> dir_pair;
    if (dir_pair_map.count(parent) == 0)
        dir_pair_map[parent] = dir_pair;
    std::string filename = name;
    dir_pair_map[parent][filename] = child;
    return OK;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    lookup(parent, name, found, ino_out);
    if (found) r = EXIST;
    else {
        ec->create(extent_protocol::T_FILE, ino_out);
        addtoparent(parent, name, ino_out);
        file_type_map[ino_out] = FILE;
        fileinfo fin;
        fin.size = 0; fin.mtime = fin.ctime = fin.atime = (unsigned)std::time(0);
        file_attr_map[ino_out] = fin;
    }
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    lookup(parent, name, found, ino_out);
    if (found) r = EXIST;
    else {
        ec->create(extent_protocol::T_DIR, ino_out);
        addtoparent(parent, name, ino_out);
        file_type_map[ino_out] = DIR;
        dirinfo din;
        din.atime = din.mtime = din.ctime = (unsigned)std::time(0);
        dir_attr_map[ino_out] = din;
    }
    return r;
}

int
yfs_client::mksym(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    bool found = false;
    lookup(parent, name, found, ino_out);
    if (found) r = EXIST;
    else {
        ec->create(extent_protocol::T_SYMLINK, ino_out);
        addtoparent(parent, name, ino_out);
        file_type_map[ino_out] = SYMLINK;
        symlinkinfo sin;
        sin.size = 0; sin.mtime = sin.atime = sin.ctime = (unsigned)std::time(0);
        symlink_attr_map[ino_out] = sin;
    }
    return r;
}

//int
//yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
//{
//    int r = OK;
//    /*
//     * your code goes here.
//     * note: lookup file from parent dir according to name;
//     * you should design the format of directory content.
//     */
//    // name,inum:name,inum:name,inum:name,inum: ......
//    std::list<dirent> list;
//    readdir(parent, list);
//    for (std::list<dirent>::iterator it=list.begin(); it!=list.end(); ++it) {
//        if (it->name.compare(name) == 0) {
//            found = true;
//            ino_out = it->inum;
//            return r;
//        }
//    }
//    found = false;
//    return r;
//}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out) {
    int r = OK;
    std::string filename = name;
    if (dir_pair_map.count(parent) == 0) found = false;
    else if (dir_pair_map[parent].count(filename) == 0) found = false;
    else {
        found = true;
        ino_out = dir_pair_map[parent][filename];
    }
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;
    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
//    std::string buf;
//    ec->get(dir, buf);
//    int start = 0, end = -1, len = buf.length();
//    for (;;) {
//        std::string name, inum_str;
//        inum inum;
//        if (start >= len) break;
//        end = buf.find(",",start);
//        name = buf.substr(start, end - start);
//        start = end + 1;
//        end = buf.find(":",start);
//        inum_str = buf.substr(start, end - start);
//        inum = n2i(inum_str);
//        dirent entry;
//        entry.name = name;entry.inum = inum;
//        list.push_back(entry);
//        start = end + 1;
//    }
    if (dir_pair_map.count(dir) == 0) return r;

    dir_attr_map[dir].atime = (unsigned)std::time(0);

    std::map<std::string, yfs_client::inum>::iterator it;
    for (it=dir_pair_map[dir].begin(); it!=dir_pair_map[dir].end(); it++) {
        dirent entry;
        entry.name = it->first; entry.inum = it->second;
        list.push_back(entry);
    }
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;
    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string buf;

    ec->get(ino, buf);
    filetype filetype = file_type_map[ino];
    if (filetype == FILE) file_attr_map[ino].atime = (unsigned)std::time(0);
    else if (filetype == DIR) dir_attr_map[ino].atime = (unsigned)std::time(0);
    else symlink_attr_map[ino].atime = (unsigned)std::time(0);

    if (off > buf.size()) data = "";
    else if (off+size > buf.size()) data=buf.substr(off);
    else data=buf.substr(off, size);
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;
    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string buf;

    ec->get(ino, buf);
    filetype filetype = file_type_map[ino];
    if (filetype == FILE) file_attr_map[ino].atime = (unsigned)std::time(0);
    else if (filetype == DIR) dir_attr_map[ino].atime = (unsigned)std::time(0);
    else symlink_attr_map[ino].atime = (unsigned)std::time(0);

    if (off+size > buf.size()) buf.resize(off+size);
    for (int i=off;i<off+size;i++)
      buf[i]=data[i-off];

    ec->put(ino, buf);
    if (filetype == FILE) {
        fileinfo fin;
        fin.mtime = fin.ctime = fin.atime = (unsigned)std::time(0);
        file_attr_map[ino] = fin;
    }
    else if (filetype == SYMLINK) {
        symlinkinfo sin;
        sin.mtime = sin.ctime = sin.atime = (unsigned) std::time(0);
        symlink_attr_map[ino] = sin;
    }
    else {
        dirinfo din;
        din.mtime = din.ctime = din.atime = (unsigned) std::time(0);
        dir_attr_map[ino] = din;
    }

    bytes_written = size;
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;
    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    std::string buf;

    ec->get(parent, buf);
    dir_attr_map[parent].atime = (unsigned)std::time(0);

//    size_t filename_pos = buf.find(name);
//    size_t inum_pos = buf.find(",", filename_pos); inum_pos++;
//    size_t end_pos = buf.find(":", inum_pos);
//    ec->remove(atoi(buf.substr(inum_pos, end_pos-inum_pos).c_str()));
//    buf.erase(filename_pos, end_pos-filename_pos+1);
    ec->put(parent, buf);
    dirinfo din;
    din.mtime = din.ctime = din.atime = (unsigned) std::time(0);
    dir_attr_map[parent] = din;

    std::string filename = name;
    yfs_client::inum inum = dir_pair_map[parent][filename];
    dir_pair_map[parent].erase(filename);
    ec->remove(inum);
    return r;
}

int yfs_client::symlink(const char *link, inum parent, const char *name, inum &ino_out) {
    int r = OK;
    size_t length = strlen(link), bytes_written;
    inum ino;
    r = mksym(parent, name, 0, ino);
    if (r != EXIST) {
        r = ec->put(ino, std::string(link));
        symlinkinfo sin;
        sin.mtime = sin.ctime = sin.atime = (unsigned) std::time(0);
        symlink_attr_map[ino] = sin;
    }
    ino_out = ino;
    return r;
}

int yfs_client::readlink(inum ino, std::string &buf) {
    int r = OK;

    ec->get(ino, buf);
    symlink_attr_map[ino].atime = (unsigned)std::time(0);

    return r;
}
