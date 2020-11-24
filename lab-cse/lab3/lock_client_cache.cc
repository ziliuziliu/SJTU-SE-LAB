// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  int result = lock_protocol::OK;
  int r;
  // Your lab2 part3 code goes here
  //tprintf("Acquiring lock %d by thread %d\n", lid, pthread_self());
  pthread_mutex_lock(&mutex);
  if (locks.count(lid) == 0) {
      locks[lid] = NONE;
      pthread_cond_t con = PTHREAD_COND_INITIALIZER;
      cvs[lid] = con;
      revoke_flag[lid] = false;
  }
  if (locks[lid] == NONE) {
ACQUIRE_LOCK_FROM_SERVER:
      locks[lid] = ACQUIRING;
      pthread_mutex_unlock(&mutex);
      lock_protocol::status ret = cl->call(lock_protocol::acquire, lid, id, r);
      pthread_mutex_lock(&mutex);
      if (ret == lock_protocol::RETRY) {
          //retry handler to wake up
          while (locks[lid] != FREE && locks[lid] != LOCKED) {
              if (locks[lid] == NONE) // might be revoked: re-acquire from server
                  goto ACQUIRE_LOCK_FROM_SERVER;
              pthread_cond_wait(&cvs[lid], &mutex);
          }
      }
      locks[lid] = LOCKED;
      pthread_mutex_unlock(&mutex);
      return result;
  }
  while (locks[lid] != FREE) {
      if (locks[lid] == NONE) // might be revoked: re-acquire from server
          goto ACQUIRE_LOCK_FROM_SERVER;
      pthread_cond_wait(&cvs[lid], &mutex);
  }
  locks[lid] = LOCKED;
  pthread_mutex_unlock(&mutex);
  return result;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  int result = lock_protocol::OK;
  // Your lab2 part3 code goes here
  //tprintf("Releasing lock %d by thread %d\n", lid, pthread_self());
  pthread_mutex_lock(&mutex);
  if (revoke_flag[lid]) { // revoke lock at once
      int r;
      locks[lid] = RELEASING;
      pthread_mutex_unlock(&mutex);
      cl->call(lock_protocol::release, lid, id, r);
      pthread_mutex_lock(&mutex);
      locks[lid] = NONE;
      revoke_flag[lid] = false;
      // tell other thread to re-acquire lock from server, or they'll sleep permanently
      pthread_cond_signal(&cvs[lid]);
      pthread_mutex_unlock(&mutex);
      return result;
  }
  locks[lid] = FREE;
  // wake up another thread
  pthread_cond_signal(&cvs[lid]);
  pthread_mutex_unlock(&mutex);
  return result;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  int result = rlock_protocol::OK;
  // Your lab2 part3 code goes here
  //tprintf("Revoking lock %d by thread %d\n", lid, pthread_self());
  pthread_mutex_lock(&mutex);
  if (locks[lid] == FREE) { // release at once
      int r;
      locks[lid] = RELEASING;
      pthread_mutex_unlock(&mutex);
      cl->call(lock_protocol::release, lid, id, r);
      pthread_mutex_lock(&mutex);
      locks[lid] = NONE;
      // tell other thread to re-acquire lock from server, or they'll sleep permanently
      pthread_cond_signal(&cvs[lid]);
      pthread_mutex_unlock(&mutex);
      return result;
  }
  revoke_flag[lid] = true; // release lock to server once finish working
  pthread_mutex_unlock(&mutex);
  return rlock_protocol::RETRY; // lock releasing unsuccessfully
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  int result = rlock_protocol::OK;
  // Your lab2 part3 code goes here
  //tprintf("Retrying lock %d by thread %d\n", lid, pthread_self());
  pthread_mutex_lock(&mutex);
  if (locks[lid] == NONE) {
    pthread_mutex_unlock(&mutex);
    return rlock_protocol::OK;
  }
  // lock granted
  if (locks[lid] == ACQUIRING) {
      locks[lid] = LOCKED;
      pthread_cond_signal(&cvs[lid]);
  }
  pthread_mutex_unlock(&mutex);
  return result;
}
