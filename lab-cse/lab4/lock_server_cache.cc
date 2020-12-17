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
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &)
{
  lock_protocol::status result = lock_protocol::OK;
  // Your lab2 part3 code goes here
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
      // revoke from previous client
      int r;
      pthread_mutex_unlock(&mutex);
      ret = h.safebind()->call(rlock_protocol::revoke, lid, r);
  }
  return ret == rlock_protocol::OK ? lock_protocol::OK : lock_protocol::RETRY;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &)
{
  lock_protocol::status result = lock_protocol::OK;
  // Your lab2 part3 code goes here
  pthread_mutex_lock(&mutex);
  locks[lid].pop();
  if (locks[lid].empty()) {
      pthread_mutex_unlock(&mutex);
      return result;
  }
  std::string top_client_dst = locks[lid].front();
  handle h(top_client_dst);
  if (h.safebind()) {
      int r;
      pthread_mutex_unlock(&mutex);
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
