#include "ydb_server.h"
#include "extent_client.h"

//#define DEBUG 1


ydb_server::ydb_server(std::string extent_dst, std::string lock_dst) {
	ec = new extent_client(extent_dst);
	lc = new lock_client(lock_dst);
	//lc = new lock_client_cache(lock_dst);

	for(int i = 2; i < 1024; i++) {    // for simplicity, just pre alloc all the needed inodes
		extent_protocol::extentid_t id;
		ec->create(extent_protocol::T_FILE, id);
	}
}

ydb_server::~ydb_server() {
	delete lc;
	delete ec;
}


ydb_protocol::status ydb_server::transaction_begin(int, ydb_protocol::transaction_id &out_id) {    // the first arg is not used, it is just a hack to the rpc lib
	// no imply, just return OK
	out_id = 1;
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::transaction_commit(ydb_protocol::transaction_id id, int &) {
	// no imply, just return OK
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::transaction_abort(ydb_protocol::transaction_id id, int &) {
	// no imply, just return OK
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::get(ydb_protocol::transaction_id id, const std::string key, std::string &out_value) {
	// lab3: your code here
	std::cout << "GET " << key << " " << out_value << std::endl;
	if (mp.count(key) != 1) return ydb_protocol::KEYINV;
    ec->get(mp[key], out_value);
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::set(ydb_protocol::transaction_id id, const std::string key, const std::string value, int &) {
	// lab3: your code here
	std::cout << "SET " << key << " " << value << std::endl;
	if (mp.count(key)) ec->put(mp[key], value);
	else {
        for (int i = 2; i < 1024; i++) {
            if (vis[i]) continue;
            vis[i] = true;
            mp[key] = i;
            ec->put(mp[key], value);
            break;
        }
    }
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::del(ydb_protocol::transaction_id id, const std::string key, int &) {
	// lab3: your code here
	std::cout << "DEL " << key << std::endl;
	if (mp.count(key) != 1) return ydb_protocol::TRANSIDINV;
	vis[mp[key]] = false; mp.erase(key);
	return ydb_protocol::OK;
}
