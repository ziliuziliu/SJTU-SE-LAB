#include "ydb_server_2pl.h"
#include "extent_client.h"

//#define DEBUG 1
extent_protocol::extentid_t ydb_server_2pl::SDBMHash(const std::string key) {
    unsigned int hash = 0;
    const char *str=key.c_str();
    while (*str)
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
    return (hash & 0x3FF)+2;
}

ydb_server_2pl::ydb_server_2pl(std::string extent_dst, std::string lock_dst) : ydb_server(extent_dst, lock_dst) {
    ks_mutex = PTHREAD_MUTEX_INITIALIZER;
    for (int i=2;i<1024;i++) wait_graph[i] = std::map<int, int>();
}

ydb_server_2pl::~ydb_server_2pl() {
}

bool ydb_server_2pl::cy_query() {
    std::map<int, int> deg = degree;
    std::queue<int> q;
    for (std::map<int, int>::iterator it=deg.begin(); it!=deg.end(); it++) {
        if (!it->second) q.push(it->first);
    }
    while (!q.empty()) {
        int head = q.front(); q.pop();
        std::map<int, int> to = wait_graph[head];
        for (std::map<int, int>::iterator it=to.begin(); it!=to.end(); it++) {
            int pos = it->first;
            deg[pos]--;
            if (!deg[pos]) q.push(pos);
        }
    }
    for (std::map<int, int>::iterator it=deg.begin(); it!=deg.end(); it++) {
        if (it->second) return true;
    }
    return false;
}

void ydb_server_2pl::delete_node(int x) {
    for (std::map<int, std::map<int, int> >::iterator it=wait_graph.begin(); it!=wait_graph.end(); it++)
        wait_graph[it->first].erase(x);
    wait_graph.erase(x);
}

void ydb_server_2pl::deadlock_abort(ydb_protocol::transaction_id id) {
    for (std::map<int, std::string>::iterator it=kv_store[id].begin(); it!=kv_store[id].end(); it++)
        lc->release(it->first);
    delete_node(id+10000);
    kv_store.erase(id);
}

ydb_protocol::status ydb_server_2pl::transaction_begin(int, ydb_protocol::transaction_id &out_id) {    // the first arg is not used, it is just a hack to the rpc lib
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
    out_id = trans_id_cnt++;
    kv_store[out_id] = std::map<int, std::string>();
    wait_graph[out_id+10000] = std::map<int, int>();
    pthread_mutex_unlock(&ks_mutex);
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::transaction_commit(ydb_protocol::transaction_id id, int &) {
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
	if (!kv_store.count(id)) {
	    pthread_mutex_unlock(&ks_mutex);
	    return ydb_protocol::TRANSIDINV;
	}
	for (std::map<int, std::string>::iterator it=kv_store[id].begin(); it!=kv_store[id].end(); it++) {
	    ec->put(it->first, it->second);
        lc->release(it->first);
	}
	delete_node(id+10000);
	kv_store.erase(id);
	pthread_mutex_unlock(&ks_mutex);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::transaction_abort(ydb_protocol::transaction_id id, int &) {
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
	if (!kv_store.count(id)) {
	    pthread_mutex_unlock(&ks_mutex);
	    return ydb_protocol::TRANSIDINV;
	}
	for (std::map<int, std::string>::iterator it=kv_store[id].begin(); it!=kv_store[id].end(); it++)
	    lc->release(it->first);
	delete_node(id+10000);
	kv_store.erase(id);
	pthread_mutex_unlock(&ks_mutex);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::get(ydb_protocol::transaction_id id, const std::string key, std::string &out_value) {
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
	extent_protocol::extentid_t ex_id = SDBMHash(key);
	if (!kv_store.count(id)) {
	    pthread_mutex_unlock(&ks_mutex);
	    return ydb_protocol::TRANSIDINV;
	}
	if (!kv_store[id].count(ex_id)) {
	    if (cy_query()) {
	        deadlock_abort(id);
	        pthread_mutex_unlock(&ks_mutex);
	        return ydb_protocol::ABORT;
	    }
	    wait_graph[id][ex_id] = 1;
	    pthread_mutex_unlock(&ks_mutex);
	    lc->acquire(ex_id);
	    pthread_mutex_lock(&ks_mutex);
	    wait_graph[ex_id][id] = 1;
	    ec->get(ex_id, out_value);
	    kv_store[id][ex_id] = out_value;
	}
	else out_value = kv_store[id][ex_id];
	pthread_mutex_unlock(&ks_mutex);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::set(ydb_protocol::transaction_id id, const std::string key, const std::string value, int &) {
	// lab3: your code here
	pthread_mutex_lock(&ks_mutex);
	extent_protocol::extentid_t ex_id = SDBMHash(key);
	if (!kv_store.count(id)) {
        pthread_mutex_unlock(&ks_mutex);
        return ydb_protocol::TRANSIDINV;
	}
	if (!kv_store[id].count(ex_id)) {
        if (cy_query()) {
            deadlock_abort(id);
            pthread_mutex_unlock(&ks_mutex);
            return ydb_protocol::ABORT;
        }
        wait_graph[id][ex_id] = 1;
        pthread_mutex_unlock(&ks_mutex);
	    lc->acquire(ex_id);
        pthread_mutex_lock(&ks_mutex);
        wait_graph[ex_id][id] = 1;
	    kv_store[id][ex_id] = value;
	}
	else kv_store[id][ex_id] = value;
	pthread_mutex_unlock(&ks_mutex);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::del(ydb_protocol::transaction_id id, const std::string key, int &r) {
	// lab3: your code here
	return set(id, key, "", r);
}