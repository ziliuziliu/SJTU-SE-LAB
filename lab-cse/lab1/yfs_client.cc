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

yfs_client::yfs_client()
{
    ec = new extent_client();

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
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    }
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 *
 * */

bool
yfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
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
    buf.resize(size);
    ec->put(ino, buf);
    return r;
}

int
yfs_client::addtoparent(inum parent, const char *name, inum child)
{
    std::string buf;
    ec->get(parent, buf);
    buf.append(name);buf.append(",");
    buf.append(filename(child));buf.append(":");
    ec->put(parent, buf);
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
    }
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;
    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    // name,inum:name,inum:name,inum:name,inum: ......
    std::list<dirent> list;
    readdir(parent, list);
    for (std::list<dirent>::iterator it=list.begin(); it!=list.end(); ++it) {
        if (it->name.compare(name) == 0) {
            found = true;
            ino_out = it->inum;
            return r;
        }
    }
    found = false;
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
    std::string buf;
    ec->get(dir, buf);
    int start = 0, end = -1, len = buf.length();
    for (;;) {
        std::string name, inum_str;
        inum inum;
        if (start >= len) break;
        end = buf.find(",",start);
        name = buf.substr(start, end - start);
        start = end + 1;
        end = buf.find(":",start);
        inum_str = buf.substr(start, end - start);
        inum = n2i(inum_str);
        dirent entry;
        entry.name = name;
        entry.inum = inum;
        list.push_back(entry);
        start = end + 1;
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
    printf("Reading\n");
    printf("%d\n",ino);
    printf("%d %d\n",off, size);
    printf("bufsize: %d\n",buf.size());
    if (off > buf.size()) data = "";
    else if (off+size > buf.size()) data=buf.substr(off);
    else data=buf.substr(off, size);
    printf("%s\n",data.c_str());
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
    printf("Writing\n");
    printf("%d\n",ino);
    printf("%d %d\n",off, size);
    if (off+size > buf.size()) buf.resize(off+size);
    for (int i=off;i<off+size;i++)
      buf[i]=data[i-off];
    printf("size: %d\n",buf.size());
    printf("%d\n",strlen(data));
    printf("%s\n",buf.c_str());
    ec->put(ino, buf);
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

    return r;
}

