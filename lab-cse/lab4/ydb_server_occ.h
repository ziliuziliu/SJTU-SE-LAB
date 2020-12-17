#ifndef ydb_server_occ_h
#define ydb_server_occ_h

#include <string>
#include <map>
#include <pthread.h>
#include "extent_client.h"
#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"
#include "ydb_protocol.h"
#include "ydb_server.h"

class ydb_server_occ: public ydb_server {
public:
    struct kv_cache {
        std::map<int, std::string> value;
        std::map<int, int> write_set;
        bool valid = true;
    };
    std::map<int, kv_cache *> kv_store;
    int transid_cnt = 0;
    pthread_mutex_t ks_mutex;

	ydb_server_occ(std::string, std::string);
	~ydb_server_occ();
    extent_protocol::extentid_t SDBMHash(const std::string key);
	ydb_protocol::status transaction_begin(int, ydb_protocol::transaction_id &);
	ydb_protocol::status transaction_commit(ydb_protocol::transaction_id, int &);
	ydb_protocol::status transaction_abort(ydb_protocol::transaction_id, int &);
	ydb_protocol::status get(ydb_protocol::transaction_id, const std::string, std::string &);
	ydb_protocol::status set(ydb_protocol::transaction_id, const std::string, const std::string, int &);
	ydb_protocol::status del(ydb_protocol::transaction_id, const std::string, int &);
};

#endif

