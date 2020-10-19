// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache()
{
  mutex = PTHREAD_MUTEX_INITIALIZER;
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &)
{
  lock_protocol::status result = lock_protocol::OK;
  // Your lab2 part3 code goes here
  tprintf("Acquiring lock %d for %s\n", lid, id.c_str());
  pthread_mutex_lock(&mutex);
  if (locks.count(lid) == 0) {
      std::queue<std::string> client_list;
      locks[lid] = client_list;
  }
  if (locks[lid].empty()) {
    locks[lid].push(id);
    pthread_mutex_unlock(&mutex);
    return result;
  }
  rlock_protocol::status ret = rlock_protocol::OK;
  std::string pre_client_dst = locks[lid].back();
  handle h(pre_client_dst);
  locks[lid].push(id);
  if (h.safebind()) {
      int r;
      pthread_mutex_unlock(&mutex);
      tprintf("Revoking lock %d for client %s\n", lid, pre_client_dst.c_str());
      ret = h.safebind()->call(rlock_protocol::revoke, lid, r);
  }
  return ret == rlock_protocol::OK ? lock_protocol::OK : lock_protocol::RETRY;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &)
{
  lock_protocol::status result = lock_protocol::OK;
  // Your lab2 part3 code goes here
  tprintf("Releasing lock %d for %s\n", lid, id.c_str());
  pthread_mutex_lock(&mutex);
  locks[lid].pop();
  std::string top_client_dst = locks[lid].front();
  handle h(top_client_dst);
  if (h.safebind()) {
      int r;
      pthread_mutex_unlock(&mutex);
      tprintf("Retrying lock %d for client %s\n", lid, top_client_dst.c_str());
      h.safebind()->call(rlock_protocol::retry, lid, r);
      return result;
  }
  return result;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}
