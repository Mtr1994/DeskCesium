#include "BaseCommand.h"
#include "./Proto/sidescansource.pb.h"
#include "mysqlconnectionpool.h"

#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>

std::string errorMessage = "消息数据异常，请联系管理员";
std::string errorParseMessage = "消息解析失败，请联系管理员";
std::string errorConnMySQL = "连接数据库失败，请联系管理员";
std::string errorQueryMySQL = "数据库查询失败，请联系管理员";
std::string errorExecMySQL = "数据库语句执行失败，自动回滚";
std::string errorCommitMySQL = "数据库事物提交失败";
std::string errorRollbackMySQL = "数据库事物回滚失败，请联系管理员";

void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_ = session_service;

    // 得到服务器的所有监听消息
    std::vector<CConfigNetIO> io_list;
    session_service_->get_server_listen_info(io_list, EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);
    for (const auto& io_type : io_list)
    {
        PSS_LOGGER_DEBUG("[CBaseCommand::Init]tcp listen {0}:{1}", io_type.ip_, io_type.port_);
    }

    session_service_->get_server_listen_info(io_list, EM_CONNECT_IO_TYPE::CONNECT_IO_UDP);
    for (const auto& io_type : io_list)
    {
        PSS_LOGGER_DEBUG("[CBaseCommand::Init]udp listen {0}:{1}", io_type.ip_, io_type.port_);
    }

    session_service_->get_server_listen_info(io_list, EM_CONNECT_IO_TYPE::CONNECT_IO_TTY);
    for (const auto& io_type : io_list)
    {
        PSS_LOGGER_DEBUG("[CBaseCommand::Init]tty listen {0}:{1}", io_type.ip_, io_type.port_);
    }

    PSS_LOGGER_DEBUG("[load_module]({0})io thread count.", session_service_->get_io_work_thread_count());
    
    // init mysql connection pool
    mMysqlConnectionPool = MySQLConnectionPool::getConnectPool();
    if (nullptr == mMysqlConnectionPool) 
    {
		PSS_LOGGER_DEBUG("create mysql connection poll failed");
	}
}

void CBaseCommand::logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, connect", source.connect_id_);
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, local ip={} local port={}", source.connect_id_, source.local_ip_.m_strClientIP, source.local_ip_.m_u2Port);
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, remote ip={} remote port={}", source.connect_id_, source.remote_ip_.m_strClientIP, source.remote_ip_.m_u2Port);
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, work thread id={}", source.connect_id_, source.work_thread_id_);

    if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_TCP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_TCP", source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_UDP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_UDP", source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_TTY)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_TTY", source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_TCP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_SERVER_TCP", source.connect_id_);
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, server_id={}", source.connect_id_, source.connect_mark_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_UDP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_SERVER_UDP", source.connect_id_);
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, server_id={}", source.connect_id_, source.connect_mark_id_);
    }
}

void CBaseCommand::logic_disconnect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[do_module_message]connand={}, disconnect", source.connect_id_);
}

void CBaseCommand::logic_insert_side_scan_source_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("logic_insert_side_scan_source_data");
    
    std::string sourceData = recv_packet->buffer_.substr(8);
    if (sourceData.empty())
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorMessage, recv_packet->command_id_);
        return;
    }
    
    SideScanSourceList sourceDataList;
    bool status = sourceDataList.ParseFromString(sourceData);
    
    if (!status)
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorParseMessage, recv_packet->buffer_);
		StatusResponse response;
		response.set_status(false);
		response.set_message(errorParseMessage);
		std::string pack = createPackage(CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
		return;
    }

	StatusResponse response;
	response.set_status(false);

    shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();
    status = conn->transaction();
    if (!status)
    {
		response.set_message(conn->lastError());
		std::string pack = createPackage(CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
        return;
	}

    uint32_t size = sourceDataList.list_size();
    char sql[1024] = { 0 };
    for (uint32_t i = 0; i < size; i++)
    {
        SideScanSource source = sourceDataList.list().at(i);

		sprintf(sql, "REPLACE INTO t_source_data_side_scan VALUES('%s', '%s', '%s', '%s', '%s', '%s', %f, %f, %f, %f, '%s', '%s', %f, %f, '%s', %f, %f, %f, %f, %d, %f, %f, '%s', '%s', %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s','%s', '%s', '%d', 0)", 
				source.id().data(), source.cruise_number().data(), source.dive_number().data(), source.scan_line().data(), source.cruise_year().data(), source.dt_time().data(), source.longitude(), source.latitude(), source.depth(),
				source.dt_speed(), source.horizontal_range_direction().data(),  source.horizontal_range_value().data(), source.height_from_bottom(), source.r_theta(), source.side_scan_image_name().data(), source.image_top_left_longitude(),
				source.image_top_left_latitude(), source.image_bottom_right_longitude(), source.image_bottom_right_latitude(), source.image_total_byte(), source.along_track(),  source.across_track(), source.remarks().data(), source.suppose_size().data(), 
				source.priority(), source.verify_auv_sss_image_paths().data(), source.verify_image_paths().data(),  source.image_description().data(),  source.target_longitude().data(), source.target_latitude().data(), 
				source.position_error().data(), source.verify_cruise_number().data(), source.verify_dive_number().data(), source.verify_time().data(), source.verify_flag());
		
		status = conn->update(sql);
		
		if (!status) break;
    }
    
    if (!status)
    {
	    std::string error = conn->lastError();
		PSS_LOGGER_DEBUG("{0} t_source_data_side_scan {1}", errorExecMySQL, error.data());
		
		// response to client
		response.set_message(error);
		std::string pack = createPackage(CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
	else
	{
		status = conn->commit();
		if (!status) 
		{
			std::string error = conn->lastError();
			PSS_LOGGER_DEBUG("{0}, {1}", errorCommitMySQL, error.data());
			response.set_message(error);
			std::string pack = createPackage(CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE, response.SerializeAsString());
		    sendAsyncPack(source.connect_id_, pack);
			
			// 事物提交失败，需要手动回滚
			status = conn->rollback();
			if (status)
			{
				PSS_LOGGER_DEBUG("数据库事物回滚成功");
			}
			else
			{
				error = conn->lastError();
				PSS_LOGGER_DEBUG("{0}, {1}", errorRollbackMySQL, error.data());
				response.set_message(error);
				std::string pack = createPackage(CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE, response.SerializeAsString());
		    	sendAsyncPack(source.connect_id_, pack);
			}
		}
		else
		{
			char tempMsg[1024] = { 0 };
			sprintf(tempMsg, "数据写入成功, 共 %d 条记录", size);
			
			response.set_status(true);
			response.set_message(tempMsg);
			std::string pack = createPackage(CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE, response.SerializeAsString());
		    sendAsyncPack(source.connect_id_, pack);
		}
	}
}

void CBaseCommand::logic_query_ftp_server_status(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
	StatusResponse response;
	response.set_status(false);
	response.set_message("文件服务未启动，请联系管理员");
		
	FILE *fp = nullptr;
    char buf[1024]={0};
    fp = popen("ps -ef | grep ftp | grep -v grep", "r");
    if(fp) 
    {
        int ret = fread(buf, 1, sizeof(buf) - 1, fp);
        if(ret > 0)
        {
			response.set_status(true);
			response.set_message("文件服务已就绪");
        }
        pclose(fp);
    }
    
    std::string pack = createPackage(CMD_QUERY_FTP_SERVER_STATUS_RESPONSE, response.SerializeAsString());
    sendAsyncPack(source.connect_id_, pack);
}

void CBaseCommand::logic_insert_cruise_route_source_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
	PSS_LOGGER_DEBUG("logic_insert_cruise_route_source_data.");
	
	std::string sourceData = recv_packet->buffer_.substr(8);
    if (sourceData.empty())
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorMessage, recv_packet->command_id_);
        return;
    }
    
    CruiseRouteSourceList sourceDataList;
    bool status = sourceDataList.ParseFromString(sourceData);
    
    if (!status)
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorParseMessage, recv_packet->buffer_);
		StatusResponse response;
		response.set_status(false);
		response.set_message(errorParseMessage);
		std::string pack = createPackage(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
		return;
    }
   
	StatusResponse response;
	response.set_status(false);

    shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();
    status = conn->transaction();
    if (!status)
    {
		response.set_message(conn->lastError());
		std::string pack = createPackage(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
        return;
	}

    uint32_t size = sourceDataList.list_size();
    char sql[1024] = { 0 };
    for (uint32_t i = 0; i < size; i++)
    {
        CruiseRouteSource source = sourceDataList.list().at(i);

		sprintf(sql, "REPLACE INTO t_source_data_cruise_route VALUES('%s', '%s', '%s', 0)",  source.cruise().data(), source.type().data(), source.name().data());
		status = conn->update(sql);
		
		if (!status) break;
    }
    
    if (!status)
    {
	    std::string error = conn->lastError();
		PSS_LOGGER_DEBUG("{0} t_source_data_cruise_route {1}", errorExecMySQL, error.data());
		
		// response to client
		response.set_message(error);
		std::string pack = createPackage(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
	else
	{
		status = conn->commit();
		if (!status) 
		{
			std::string error = conn->lastError();
			PSS_LOGGER_DEBUG("{0}, {1}", errorCommitMySQL, error.data());
			response.set_message(error);
			std::string pack = createPackage(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE, response.SerializeAsString());
		    sendAsyncPack(source.connect_id_, pack);
			
			status = conn->rollback();
			if (status)
			{
				PSS_LOGGER_DEBUG("数据库事物回滚成功");
			}
			else
			{
				error = conn->lastError();
				PSS_LOGGER_DEBUG("{0}, {1}", errorRollbackMySQL, error.data());
				response.set_message(error);
				std::string pack = createPackage(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE, response.SerializeAsString());
		    	sendAsyncPack(source.connect_id_, pack);
			}
		}
		else
		{
			char tempMsg[1024] = { 0 };
			sprintf(tempMsg, "数据写入成功, 共 %d 条记录", size);
			
			response.set_status(true);
			response.set_message(tempMsg);
			std::string pack = createPackage(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE, response.SerializeAsString());
		    sendAsyncPack(source.connect_id_, pack);
		}
	}
}

void CBaseCommand::logic_query_search_filter_parameter_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
	PSS_LOGGER_DEBUG("logic_query_search_filter_parameter_data.");
	
	StatusResponse response;
	response.set_status(false);

	shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

    char sql[1024] = { 0 };
    sprintf(sql, "select distinct cruise_year, cruise_number, dive_number, verify_dive_number from t_source_data_side_scan where status_flag = 0;");
	bool status = conn->query(sql);
    
    if (!status)
    {
	    std::string error = conn->lastError();
		PSS_LOGGER_DEBUG("{0} t_source_data_side_scan {1}", errorQueryMySQL, error.data());
		
		// response to client
		response.set_message(error);
		std::string pack = createPackage(CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
	else
	{
		SearchFilterParamterList sourceList;
		while (conn->next())
    	{
			SearchFilterParamter *source = sourceList.add_list();
			source->set_cruise_year(conn->value(0));
			source->set_cruise_number(conn->value(1));
			source->set_dive_number(conn->value(2));
			source->set_verify_dive_number(conn->value(3));
    	}
    	
		std::string pack = createPackage(CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE, sourceList.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
}

void CBaseCommand::logic_query_side_scan_source_data_by_filter(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
	PSS_LOGGER_DEBUG("logic_query_side_scan_source_data_by_filter");
	
	std::string parameter = recv_packet->buffer_.substr(8);
    if (parameter.empty())
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorMessage, recv_packet->command_id_);
        return;
    }
    
    FilterSearchParameter searchParameter;
    bool status = searchParameter.ParseFromString(parameter);
    
    if (!status)
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorParseMessage, recv_packet->buffer_);
		StatusResponse response;
		response.set_status(false);
		response.set_message(errorParseMessage);
		std::string pack = createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
		return;
    }
	
	StatusResponse response;
	response.set_status(false);

	shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

    std::string sql = "select * from t_source_data_side_scan where ";
    if (searchParameter.cruise_year().length() > 2) 
    {
		sql.append("cruise_year in (" + searchParameter.cruise_year() +")");
	}
	
    if (searchParameter.cruise_number().length() > 2) 
    {
    	if (sql.length() > 44) sql.append(" and ");
		sql.append("cruise_number in (" + searchParameter.cruise_number() +")");
	}
	
    if (searchParameter.dive_number().length() > 2) 
    {
	    if (sql.length() > 44) sql.append(" and ");
		sql.append("dive_number in (" + searchParameter.dive_number() +")");
	}
	
	uint16_t verifyDiveNumberSize = searchParameter.verify_dive_number_size();
	if (verifyDiveNumberSize > 0) if (sql.length() > 44) sql.append(" and ");
	for (uint16_t i = 0; i < verifyDiveNumberSize; i++)
	{
		if ((i == 0) && (verifyDiveNumberSize > 1)) sql.append("(");
		if (i > 0) sql.append(" or ");
		sql.append("verify_dive_number like (\"%" + searchParameter.verify_dive_number(i) +"%\")");
		if ((verifyDiveNumberSize > 1) && (verifyDiveNumberSize == (i + 1))) sql.append(")");
	}
	
	if (searchParameter.priority().length() > 0) 
    {
    	if (sql.length() > 44) sql.append(" and ");
		sql.append("priority in (" + searchParameter.priority() +")");
	}
	
	if (searchParameter.verify_flag().length() > 0) 
    {
    	if (sql.length() > 44) sql.append(" and ");
		sql.append("verify_flag in (" + searchParameter.verify_flag() +")");
	}
	sql.append(" and status_flag = 0;");
    
    PSS_LOGGER_DEBUG("sql {0}", sql.data());
    
	status = conn->query(sql);
    
    if (!status)
    {
	    std::string error = conn->lastError();
		PSS_LOGGER_DEBUG("{0} t_source_data_side_scan {1}", errorQueryMySQL, error.data());
		
		// response to client
		response.set_message(error);
		std::string pack = createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
	else
	{
		SideScanSourceList sourceList;
		while (conn->next())
    	{
			SideScanSource *source = sourceList.add_list();
			uint16 index = 0;
			source->set_id(conn->value(index++));
			source->set_cruise_number(conn->value(index++));
			source->set_dive_number(conn->value(index++));
			source->set_scan_line(conn->value(index++));
			source->set_cruise_year(conn->value(index++));
			source->set_dt_time(conn->value(index++));
			source->set_longitude(stod(conn->value(index++)));
			source->set_latitude(stod(conn->value(index++)));
			source->set_depth(stod(conn->value(index++)));
			source->set_dt_speed(stof(conn->value(index++)));
			source->set_horizontal_range_direction(conn->value(index++));
			source->set_horizontal_range_value(conn->value(index++));
			source->set_height_from_bottom(stof(conn->value(index++)));
			source->set_r_theta(stof(conn->value(index++)));
			source->set_side_scan_image_name(conn->value(index++));
			source->set_image_top_left_longitude(stod(conn->value(index++)));
			source->set_image_top_left_latitude(stod(conn->value(index++)));
			source->set_image_bottom_right_longitude(stod(conn->value(index++)));
			source->set_image_bottom_right_latitude(stod(conn->value(index++)));
			source->set_image_total_byte(stoi(conn->value(index++)));
			source->set_along_track(stof(conn->value(index++)));
			source->set_across_track(stof(conn->value(index++)));
			source->set_remarks(conn->value(index++));
			source->set_suppose_size(conn->value(index++));
			source->set_priority(stoi(conn->value(index++)));
			source->set_verify_auv_sss_image_paths(conn->value(index++));
			source->set_verify_image_paths(conn->value(index++));
			source->set_image_description(conn->value(index++));
			source->set_target_longitude(conn->value(index++));
			source->set_target_latitude(conn->value(index++));
			source->set_position_error(conn->value(index++));
			source->set_verify_cruise_number(conn->value(index++));
			source->set_verify_dive_number(conn->value(index++));
			source->set_verify_time(conn->value(index++));
			source->set_verify_flag(stoi(conn->value(index++)));
    	}
    	
		std::string pack = createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER_RESPONSE, sourceList.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
}

void CBaseCommand::logic_query_side_scan_source_data_by_keyword(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
	PSS_LOGGER_DEBUG("logic_query_side_scan_source_data_by_keyword");
	
	std::string parameter = recv_packet->buffer_.substr(8);
    if (parameter.empty())
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorMessage, recv_packet->command_id_);
        return;
    }
    
    KeywordSearchParameter searchParameter;
    bool status = searchParameter.ParseFromString(parameter);
    
    if ((!status) || (searchParameter.keyword().length() == 0))
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorParseMessage, recv_packet->buffer_);
		StatusResponse response;
		response.set_status(false);
		response.set_message(errorParseMessage);
		std::string pack = createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
		return;
    }
	
	StatusResponse response;
	response.set_status(false);

	shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

    std::string sql = "select * from t_source_data_side_scan where ";
    sql.append("(remarks like (\"%" + searchParameter.keyword() +"%\") or image_description like (\"%" + searchParameter.keyword() +"%\"))");
	
	
	sql.append(" and status_flag = 0;");
    
    PSS_LOGGER_DEBUG("sql {0}", sql.data());
    
	status = conn->query(sql);
    
    if (!status)
    {
	    std::string error = conn->lastError();
		PSS_LOGGER_DEBUG("{0} t_source_data_side_scan {1}", errorQueryMySQL, error.data());
		
		// response to client
		response.set_message(error);
		std::string pack = createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
	else
	{
		SideScanSourceList sourceList;
		while (conn->next())
    	{
			SideScanSource *source = sourceList.add_list();
			uint16 index = 0;
			source->set_id(conn->value(index++));
			source->set_cruise_number(conn->value(index++));
			source->set_dive_number(conn->value(index++));
			source->set_scan_line(conn->value(index++));
			source->set_cruise_year(conn->value(index++));
			source->set_dt_time(conn->value(index++));
			source->set_longitude(stod(conn->value(index++)));
			source->set_latitude(stod(conn->value(index++)));
			source->set_depth(stod(conn->value(index++)));
			source->set_dt_speed(stof(conn->value(index++)));
			source->set_horizontal_range_direction(conn->value(index++));
			source->set_horizontal_range_value(conn->value(index++));
			source->set_height_from_bottom(stof(conn->value(index++)));
			source->set_r_theta(stof(conn->value(index++)));
			source->set_side_scan_image_name(conn->value(index++));
			source->set_image_top_left_longitude(stod(conn->value(index++)));
			source->set_image_top_left_latitude(stod(conn->value(index++)));
			source->set_image_bottom_right_longitude(stod(conn->value(index++)));
			source->set_image_bottom_right_latitude(stod(conn->value(index++)));
			source->set_image_total_byte(stoi(conn->value(index++)));
			source->set_along_track(stof(conn->value(index++)));
			source->set_across_track(stof(conn->value(index++)));
			source->set_remarks(conn->value(index++));
			source->set_suppose_size(conn->value(index++));
			source->set_priority(stoi(conn->value(index++)));
			source->set_verify_auv_sss_image_paths(conn->value(index++));
			source->set_verify_image_paths(conn->value(index++));
			source->set_image_description(conn->value(index++));
			source->set_target_longitude(conn->value(index++));
			source->set_target_latitude(conn->value(index++));
			source->set_position_error(conn->value(index++));
			source->set_verify_cruise_number(conn->value(index++));
			source->set_verify_dive_number(conn->value(index++));
			source->set_verify_time(conn->value(index++));
			source->set_verify_flag(stoi(conn->value(index++)));
    	}
    	
		std::string pack = createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD_RESPONSE, sourceList.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
	}
}

void CBaseCommand::logic_query_trajectory_by_cruise_and_dive(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
	PSS_LOGGER_DEBUG("logic_query_trajectory_by_cruise_and_dive");
	
	RequestTrajectoryResponse response;
	response.set_status(false);
	
	std::string parameter = recv_packet->buffer_.substr(8);
    if (parameter.empty())
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorMessage, recv_packet->command_id_);
        return;
    }
    
    RequestTrajectory requestTrajectory;
    bool status = requestTrajectory.ParseFromString(parameter);
    
    if (!status)
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorParseMessage, recv_packet->buffer_);
		response.set_id(errorParseMessage);
		std::string pack = createPackage(CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
		return;
    }

	std::string dive_number = requestTrajectory.dive_number();
	std::string targetPath = "/home/ftp_root/upload/" + requestTrajectory.cruise_number() + "/Navigation/" + requestTrajectory.trajectory_type();

	std::vector<string> vectorFileNames;

    DIR *pDir;
    struct dirent* ptr;
    if(!(pDir = opendir(targetPath.c_str())))
    {
    	response.set_id("没有找到对应的轨迹文件");
		std::string pack = createPackage(CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
        return;
    }
    while((ptr = readdir(pDir)) != 0) 
    {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0 && std::string(ptr->d_name).find(".txt") != std::string::npos)
        {
            vectorFileNames.push_back(ptr->d_name);
        }
    }
    closedir(pDir);
	
	std::string position_chain = "";
	for (auto &name : vectorFileNames)
	{
		std::ifstream fileDescripter(targetPath + "/" + name);
		std::string line;
		while(std::getline(fileDescripter, line))
		{
			PSS_LOGGER_DEBUG("what");
			std::regex reg("^([0-9]+\\.[0-9]+) ([0-9]+\\.[0-9]+) (.*)");
			std::smatch match;
			bool status = regex_search(line, match, reg);
			PSS_LOGGER_DEBUG("what A {0} {1}", status, match.size());
			if (!status) continue;
			if (match.size() != 4) continue;
			
			if (position_chain.length() > 0) position_chain += " ";
			position_chain += std::string(match[1]) + "," + std::string(match[2]) + ",0";
		}
		PSS_LOGGER_DEBUG("open file 1 {0}", targetPath + "/" + name);
	}

	response.set_status(true);
	response.set_id(requestTrajectory.cruise_number() + "-" + dive_number);
	response.set_position_chain(position_chain);
	std::string pack = createPackage(CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE, response.SerializeAsString());
    sendAsyncPack(source.connect_id_, pack);
}

void CBaseCommand::sendAsyncPack(uint32_t id, const std::string& pack)
{
    if (pack.size() == 0) return;
    auto send_asyn_packet = std::make_shared<CMessage_Packet>();
    send_asyn_packet->buffer_.append(pack.c_str(), pack.size());
    session_service_->send_io_message(id, send_asyn_packet);
}

std::string CBaseCommand::createPackage(uint16_t cmd, const std::string& data)
{
    uint64_t length = data.length();
    uint16_t version = 0x1107; // month + day

    std::string pack;
    pack.resize(length + 8);
    memcpy(&pack[0], &version, 2);
    memcpy(&pack[2], &cmd, 2);
    memcpy(&pack[4], &length, 4);
    memcpy(&pack[8], data.c_str(), data.length());
    
    return pack;
}
