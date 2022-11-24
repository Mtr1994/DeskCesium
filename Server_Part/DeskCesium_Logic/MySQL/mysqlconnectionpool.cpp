#include "mysqlconnectionpool.h"
#include <fstream>
#include <thread>

MySQLConnectionPool* MySQLConnectionPool::getConnectPool()
{
    static MySQLConnectionPool pool;
    return &pool;
}

void MySQLConnectionPool::produceConnection()
{
    while (true)
    {
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize)
        {
            m_cond.wait(locker);
        }
        addConnection();
        m_cond.notify_all();
    }
}

void MySQLConnectionPool::recycleConnection()
{
    while (true)
    {
        this_thread::sleep_for(chrono::milliseconds(500));
        {
		    lock_guard<mutex> locker(m_mutexQ);
		    while (m_connectionQ.size() >= m_minSize)
		    {
		        MysqlConnection* conn = m_connectionQ.front();
		        if (conn->getAliveTime() >= m_maxIdleTime)
		        {
		            m_connectionQ.pop();
		            delete conn;
		        }
		        else
		        {
		            break;
		        }
		    }
        }
        
        m_cond.notify_all();
    }
}

void MySQLConnectionPool::addConnection()
{
    MysqlConnection* conn = new MysqlConnection;
    conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}

shared_ptr<MysqlConnection> MySQLConnectionPool::getConnection()
{
    unique_lock<mutex> locker(m_mutexQ);
    while (m_connectionQ.empty())
    {
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
        {
            if (m_connectionQ.empty())
            {
                //return nullptr;
                continue;
            }
        }
    }
    shared_ptr<MysqlConnection> connptr(m_connectionQ.front(), [this](MysqlConnection* conn) {
        lock_guard<mutex> locker(m_mutexQ);
        conn->refreshAliveTime();
        m_connectionQ.push(conn);
        });
    m_connectionQ.pop();
    m_cond.notify_all();
    return connptr;
}

MySQLConnectionPool::~MySQLConnectionPool()
{
    while (!m_connectionQ.empty())
    {
        MysqlConnection* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}

MySQLConnectionPool::MySQLConnectionPool()
{
	// config para
	m_ip = "101.34.253.220";
	m_port = 3306;
	m_user = "root";
	m_passwd = "noitom*2022";
	m_dbName = "deskcesium";
	m_minSize = 64;
	m_maxSize = 1024;
	m_maxIdleTime = 5000;
	m_timeout = 1000;

    for (int i = 0; i < m_minSize; ++i)
    {
        addConnection();
    }
	
    thread producer(&MySQLConnectionPool::produceConnection, this);
    thread recycler(&MySQLConnectionPool::recycleConnection, this);
    producer.detach();
    recycler.detach();
}
