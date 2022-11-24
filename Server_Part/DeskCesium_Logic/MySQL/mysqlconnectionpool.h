#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include "mysqlconnection.h"

using namespace std;
class MySQLConnectionPool
{
public:
    static MySQLConnectionPool* getConnectPool();
    MySQLConnectionPool(const MySQLConnectionPool& obj) = delete;
    MySQLConnectionPool& operator=(const MySQLConnectionPool& obj) = delete;
    shared_ptr<MysqlConnection> getConnection();
    ~MySQLConnectionPool();
	
private:
    MySQLConnectionPool();
    void produceConnection();
    void recycleConnection();
    void addConnection();

    string m_ip;
    string m_user;
    string m_passwd;
    string m_dbName;
    unsigned short m_port;
    int m_minSize;
    int m_maxSize;
    int m_timeout;
    int m_maxIdleTime;
    queue<MysqlConnection*> m_connectionQ;
    mutex m_mutexQ;
    condition_variable m_cond;
};

