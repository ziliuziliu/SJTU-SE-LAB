#include "ydb_server_occ.h"
#include "extent_client.h"

//#define DEBUG 1

ydb_server_occ::ydb_server_occ(std::string extent_dst, std::string lock_dst) : ydb_server(extent_dst, lock_dst) {
    ks_mutex = PTHREAD_MUTEX_INITIALIZER;
}

ydb_server_occ::~ydb_server_occ() {
}

extent_protocol::extentid_t ydb_server_occ::SDBMHash(const std::string key) {
    unsigned int hash = 0;
    const char *str=key.c_str();
    while (*str)
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
    return (hash & 0x3FF)+2;
}

ydb_protocol::status ydb_server_occ::transaction_begin(int, ydb_protocol::transaction_id &out_id) {    // the first arg is not used, it is just a hack to the rpc lib
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
	out_id = transid_cnt++;
	kv_store[out_id] = new kv_cache();
	pthread_mutex_unlock(&ks_mutex);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::transaction_commit(ydb_protocol::transaction_id id, int &) {
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
	kv_cache *cache = kv_store[id];
	if (cache->valid) {
	    for (std::map<int, int>::iterator it1=cache->write_set.begin(); it1!=cache->write_set.end(); it1++) {
	        int write_value = it1->first;
            for (std::map<int, kv_cache *>::iterator it2 = kv_store.begin(); it2 != kv_store.end(); it2++) {
                if (it2->first == id) continue;
                kv_cache *tmp = it2->second;
                if (tmp->value.count(write_value))
                    tmp->valid = false;
            }
        }
	    for (std::map<int, std::string>::iterator it=cache->value.begin(); it!=cache->value.end(); it++) {
	        if (cache->write_set.count(it->first))
	            ec->put(it->first, it->second);
	    }
	    delete cache;
	    kv_store.erase(id);
	    return ydb_protocol::OK;
	}
	else {
	    delete cache;
	    kv_store.erase(id);
	    return ydb_protocol::ABORT;
	}
}

ydb_protocol::status ydb_server_occ::transaction_abort(ydb_protocol::transaction_id id, int &) {
	// lab3: your code here
	kv_cache *cache = kv_store[id];
	delete cache;
	kv_store.erase(id);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::get(ydb_protocol::transaction_id id, const std::string key, std::string &out_value) {
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
    extent_protocol::extentid_t ex_id = SDBMHash(key);
    if (!kv_store.count(id)) {
        pthread_mutex_unlock(&ks_mutex);
        return ydb_protocol::TRANSIDINV;
    }
    kv_cache *cache = kv_store[id];
    if (!cache->value.count(ex_id)) {
        ec->get(ex_id, out_value);
        cache->value[ex_id] = out_value;
    }
    else out_value = cache->value[ex_id];
    pthread_mutex_unlock(&ks_mutex);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::set(ydb_protocol::transaction_id id, const std::string key, const std::string value, int &) {
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
	extent_protocol::extentid_t ex_id = SDBMHash(key);
	if (!kv_store.count(id)) {
	    pthread_mutex_unlock(&ks_mutex);
	    return ydb_protocol::TRANSIDINV;
	}
	kv_cache *cache = kv_store[id];
	cache->value[ex_id] = value;
	cache->write_set[ex_id] = 1;
	pthread_mutex_unlock(&ks_mutex);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::del(ydb_protocol::transaction_id id, const std::string key, int &r) {
	// lab3: your code here
	set(id, key, "", r);
	return ydb_protocol::OK;
}

