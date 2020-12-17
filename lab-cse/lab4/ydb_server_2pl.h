#ifndef ydb_server_2pl_h
#define ydb_server_2pl_h

#include <string>
#include <map>
#include <queue>
#include <pthread.h>
#include "extent_client.h"
#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"
#include "ydb_protocol.h"
#include "ydb_server.h"

class ydb_server_2pl: public ydb_server {
private:
    bool cy_query();
    void delete_node(int x);
    void deadlock_abort(ydb_protocol::transaction_id id);

public:
    int trans_id_cnt = 0;
    std::map<int, std::map<int, std::string> > kv_store;
    std::map<int, std::map<int, int> >write_set;
    std::map<int, std::map<int, int> > wait_graph;
    std::map<int, int> degree;
    pthread_mutex_t ks_mutex;

	ydb_server_2pl(std::string, std::string);
	~ydb_server_2pl();
	extent_protocol::extentid_t SDBMHash(const std::string key);
	ydb_protocol::status transaction_begin(int, ydb_protocol::transaction_id &);
	ydb_protocol::status transaction_commit(ydb_protocol::transaction_id, int &);
	ydb_protocol::status transaction_abort(ydb_protocol::transaction_id, int &);
	ydb_protocol::status get(ydb_protocol::transaction_id, const std::string, std::string &);
	ydb_protocol::status set(ydb_protocol::transaction_id, const std::string, const std::string, int &);
	ydb_protocol::status del(ydb_protocol::transaction_id, const std::string, int &);
};

#endif

