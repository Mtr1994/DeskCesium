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

	ISessionService* session_service_ = nullptr;
	
private:
	void sendAsyncPack(uint32_t id, const std::string& pack);

private:
	std::string createPackage(uint16_t cmd, const std::string &data);
};

