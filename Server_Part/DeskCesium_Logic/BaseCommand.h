#pragma once
#include <iostream>

#include "IFrameObject.h"
#include "define.h"

#include <vector>

// user defined cmd

// insert error record
const uint16_t CMD_INSERT_SIDE_SCAN_SOURCE_DATA = 0X7001;

// insert error record response
const uint16_t CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE = 0XB001;

// query ftp server status
const uint16_t CMD_QUERY_FTP_SERVER_STATUS = 0X7003;

// query ftp server status response
const uint16_t CMD_QUERY_FTP_SERVER_STATUS_RESPONSE = 0XB003;

// insert cruise data 
const uint16_t CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA = 0X7005;

// insert cruise data response
const uint16_t CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE = 0XB005;

// query search parameter source
const uint16_t CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA = 0X7007;

// query search parameter source response
const uint16_t CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE = 0XB007;

// query side scan data by filter
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER = 0X7009;

// query side scan data by filter response
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER_RESPONSE = 0XB009;

// query side scan data by keyword
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD = 0X700b;

// query side scan data by keyword response
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD_RESPONSE = 0XB00b;

// query trajectory by cruise and dive
const uint16_t CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE = 0X700d;

// query trajectory by cruise and dive response
const uint16_t CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE = 0XB00d;

class MySQLConnectionPool;
class CBaseCommand
{
public:
	void Init(ISessionService* session_service);

	void logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_disconnect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	
	// user defined cmd call back
	void logic_insert_side_scan_source_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	
	void logic_query_ftp_server_status(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	
	void logic_insert_cruise_route_source_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	
	void logic_query_search_filter_parameter_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	
	void logic_query_side_scan_source_data_by_filter(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	
	void logic_query_side_scan_source_data_by_keyword(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	
	void logic_query_trajectory_by_cruise_and_dive(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);

	ISessionService* session_service_ = nullptr;
	
private:
	void sendAsyncPack(uint32_t id, const std::string& pack);

private:
	std::string createPackage(uint16_t cmd, const std::string &data);
	
	MySQLConnectionPool *mMysqlConnectionPool = nullptr;
};

