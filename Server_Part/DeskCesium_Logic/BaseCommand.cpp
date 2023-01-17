#include "BaseCommand.h"
#include "./Proto/sidescansource.pb.h"
#include "mysqlconnectionpool.h"

#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>
#include <set>
#include <vector>

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
	
	std::vector<uint32_t> vectorCoverErrorNumber;
	
	// 先查询每一个轨迹文件覆盖的异常点数量，便于后期查询，因为上传轨迹前，异常点已经上传了，不会遗漏
	{
		uint32_t size = sourceDataList.list_size();
		for (uint32_t i = 0; i < size; i++)
		{
		    std::string mask = sourceDataList.list().at(i).name();
		    if (mask.length() <= 4) 
		    {
		    	vectorCoverErrorNumber.push_back(0);
				continue;
			}
		    mask = mask.substr(0, mask.length() - 4);

			shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();
			std:string sql = "select count(*) from t_source_data_side_scan where id like \"%" + mask + "%\" and status_flag = 0;";
			PSS_LOGGER_DEBUG("sql {0}", sql);
			status = conn->query(sql);
			
			if (!status)
			{
				vectorCoverErrorNumber.push_back(0);
				continue;
			}
			while (conn->next()) vectorCoverErrorNumber.push_back(stoi(conn->value(0)));
		}
	}
	
	shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();
	// 写入数据
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

		sprintf(sql, "REPLACE INTO t_source_data_cruise_route VALUES('%s', '%s', '%s', %f, %f, %d, 0)",  
																	source.cruise().data(), source.type().data(), 
																	source.name().data(), source.length(), source.area(), vectorCoverErrorNumber.at(i));
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
	shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

    std::string sql = "select name, length, area, cover_error_number from t_source_data_cruise_route where ";
    sql.append("cruise_number = \"" + requestTrajectory.cruise_number() + "\"");
    sql.append(" and type = \"" + requestTrajectory.trajectory_type() + "\"");
    sql.append(" and name like \"%" + requestTrajectory.dive_number() + "%\"");
	sql.append(" and status_flag = 0;");
    
    PSS_LOGGER_DEBUG("trajectory sql {0}", sql.data());
    
	status = conn->query(sql);
    
    if (!status)
    {
	    std::string error = conn->lastError();
		PSS_LOGGER_DEBUG("{0} logic_query_trajectory_by_cruise_and_dive {1}", errorQueryMySQL, error.data());
		
		// response to client
		std::string pack = createPackage(CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
        return;
	}
	
	std::vector<string> vectorFileNames;
	std::vector<string> vectorLength;
	std::vector<string> vectorArea;
	std::vector<string> vectorCoverErrorNumber;
	
	while (conn->next())
	{
		vectorFileNames.push_back(conn->value(0));
		vectorLength.push_back(conn->value(1));
		vectorArea.push_back(conn->value(2));
		vectorCoverErrorNumber.push_back(conn->value(3));
	}
	
	std::string targetPath = "/home/ftp_root/upload/" + requestTrajectory.cruise_number() + "/Navigation/" + requestTrajectory.trajectory_type();

	std::string position_chains = "position_chains: [";
	int16_t cruiseCount = -1;
	for (auto &name : vectorFileNames)
	{
		cruiseCount++;
		if (name.find(dive_number) == name.npos ) continue;
		std::ifstream fileDescripter(targetPath + "/" + name);
		PSS_LOGGER_DEBUG("open file {0}", targetPath + "/" + name);
		std::string position_chain = "";
		std::string line;
		while(std::getline(fileDescripter, line))
		{
			std::regex reg("^([0-9]+\\.[0-9]+)[ ]+([0-9]+\\.[0-9]+)[ ]+(.*)");
			std::smatch match;
			bool status = regex_search(line, match, reg);
			if (!status) continue;
			if (match.size() != 4) continue;
			
			if (position_chain.length() > 0) position_chain += " ";
			position_chain += std::string(match[1]) + "," + std::string(match[2]) + ",0";
		}
		if (position_chain.length() > 0)
		{	
			if (position_chains.length() > 19) position_chains.append(", ");
			std::string lineName = name.substr(0, name.length() - 4);
			position_chains.append("{id: \"" + lineName + "\", length: " + 
												vectorLength.at(cruiseCount) + ", area: " + 
												vectorArea.at(cruiseCount) + ", cover_error_number: " + 
												vectorCoverErrorNumber.at(cruiseCount) + ", type: \"" + 
												requestTrajectory.trajectory_type() + "\", position_chain: \"" + 
												position_chain + "\"}");
			
		}
	}
	position_chains.append("]");
	
	response.set_status(position_chains.length() > 20);
	response.set_position_chains(position_chains);

	response.set_id(requestTrajectory.cruise_number() + "-" + dive_number);
	std::string pack = createPackage(CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE, response.SerializeAsString());
    sendAsyncPack(source.connect_id_, pack);
}

void CBaseCommand::logic_query_statistics_data_by_condition(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
	PSS_LOGGER_DEBUG("logic_query_statistics_data_by_condition");
	
	RequestStatisticsResponse response;
	response.set_status(false);
	
	std::string parameter = recv_packet->buffer_.substr(8);
    if (parameter.empty())
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorMessage, recv_packet->command_id_);
        return;
    }
    
    RequestStatistics requestStatistics;
    bool status = requestStatistics.ParseFromString(parameter);
	
	if (!status)
    {
		PSS_LOGGER_DEBUG("{0}, {1}", errorParseMessage, recv_packet->buffer_);
		response.set_message(errorParseMessage);
		std::string pack = createPackage(CMD_QUERY_STATISTICS_DATA_BY_CONDITION_RESPONSE, response.SerializeAsString());
        sendAsyncPack(source.connect_id_, pack);
		return;
    }
    
    // 查询深拖信息
    if (requestStatistics.query_dt()) 
    {
    	std::string positionChains = generateCesiumPositionChain("DT");
		if (positionChains.length() > 4)
		{
			response.set_status(true);
			response.set_dt("{type: \"DT\", trajectory_data: " + positionChains + "}");
		}
    }
    
    // 查询AUV信息
    if (requestStatistics.query_auv()) 
    {
    	std::string positionChains = generateCesiumPositionChain("AUV");
    	PSS_LOGGER_DEBUG("AUV length {0}", positionChains.length());
		if (positionChains.length() > 4)
		{
			response.set_status(true);
			response.set_auv("{type: \"AUV\", trajectory_data: " + positionChains + "}");
		}
    }
    
    // 查询HOV信息
    if (requestStatistics.query_hov()) 
    {
    	std::string positionChains = generateCesiumPositionChain("HOV");
		if (positionChains.length() > 4)
		{
			response.set_status(true);
			response.set_hov("{type: \"HOV\", trajectory_data: " + positionChains + "}");
		}
    }
    
    // 查询船舶轨迹信息
    if (requestStatistics.query_ship()) 
    {
    	std::string positionChains = generateCesiumPositionChain("SHIP");
		if (positionChains.length() > 4)
		{
			response.set_status(true);
			response.set_ship("{type: \"SHIP\", trajectory_data: " + positionChains + "}");
		}
    }
    
    // 查询异常点信息
    if (requestStatistics.query_errorpoint()) 
    {
    	shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

		std::string sql = "select longitude, latitude, priority, verify_flag from t_source_data_side_scan";
		sql.append(" where status_flag = 0;");
		
		PSS_LOGGER_DEBUG("query error point sql {0}", sql.data());
		
		status = conn->query(sql);
		
		if (status)
		{
			std::string result = "{data: [";
			while (conn->next())
			{
				if (result.length() != 8) result.append(", ");
				result.append("{priority: " + 
										conn->value(2) + ", verify_flag: " + 
										conn->value(3) + ", longitude: " + 
										conn->value(0) + ", latitude: " + 
										conn->value(1) + "}");
			}
			result.append("]}");
			
			// 有一个有效信息就算成功
			response.set_status(true);
			response.set_errorpoint(result);
		}
    }
    
    // 查询前言信息
    if (requestStatistics.query_preface()) 
    {
    	std::string preface = "{sidescan: ";
		shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();
		std::string sql = "select cruise_number, priority, verify_flag, verify_dive_number from t_source_data_side_scan where status_flag = 0;";
		PSS_LOGGER_DEBUG("statistics sql {0}", sql.data());
		status = conn->query(sql);
		if (!status)
		{
			std::string error = conn->lastError();
			PSS_LOGGER_DEBUG("{0} logic_query_statistics_data_by_condition {1}", errorQueryMySQL, error.data());
			
			preface.append("{total_cruise_number: 0, total_error_number: 0, p1: 0, p2: 0, p3: 0, verify_number: 0, verify_p1: 0, verify_p2: 0, verify_p3: 0},");
		}
		else
		{
			int total_cruise_number = 0, total_error_number = 0, p1 = 0, p2 = 0, p3 = 0, verify_number= 0, verify_p1 = 0, verify_p2 = 0, verify_p3 = 0, verify_auv = 0, verify_hov = 0;
			std::set<string>  setCruiseNumber;
			while (conn->next())
			{
				setCruiseNumber.insert(conn->value(0));
				total_error_number++;
				
				bool verifyFlag = stoi(conn->value(2));
				if (stoi(conn->value(1)) == 1) 
				{
					p1++;
					if (verifyFlag)
					{
						verify_number++;
						verify_p1++;
					}
				}
				else if (stoi(conn->value(1)) == 2) 
				{
					p2++;
					if (verifyFlag)
					{
						verify_number++;
						verify_p2++;
					}
				}
				else if (stoi(conn->value(1)) == 3) 
				{
					p3++;
					if (verifyFlag)
					{
						verify_number++;
						verify_p3++;
					}
				}
				
				if (verifyFlag && (conn->value(3).find("HS") != std::string::npos))
				{
					verify_auv++;
				}
				
				if (verifyFlag && ((conn->value(3).find("FDZ") != std::string::npos) || (conn->value(3).find("SY") != std::string::npos)))
				{
					verify_hov++;
				}
			}
			
			total_cruise_number = setCruiseNumber.size();
			preface.append("{total_cruise_number: " + std::to_string(total_cruise_number) + 
							", total_error_number: " + std::to_string(total_error_number) + 
							", p1: " + std::to_string(p1) + 
							", p2: " + std::to_string(p2) + 
							", p3: " + std::to_string(p3) + 
							", verify_number: " + std::to_string(verify_number) + 
							", verify_p1: " + std::to_string(verify_p1) + 
							", verify_p2: " + std::to_string(verify_p2) + 
							", verify_p3: " + std::to_string(verify_p3) + 
							", verify_auv: " + std::to_string(verify_auv) + 
							", verify_hov: " + std::to_string(verify_hov) + "}, cruiseroute: ");
		}
		
		sql = "select type, length, area, name, cruise_number from t_source_data_cruise_route where status_flag = 0;";
		status = conn->query(sql);
		if (!status)
		{
			std::string error = conn->lastError();
			PSS_LOGGER_DEBUG("{0} logic_query_statistics_data_by_condition {1}", errorQueryMySQL, error.data());
			
			preface.append("{total_length: 0, total_area: 0, dt_total_length: 0, dt_total_area: 0, auv_total_length: 0, auv_total_area: 0, hov_total_length: 0, hov_total_area: 0}}");
		}
		else
		{
			float total_length = 0, total_area = 0, total_hov_number = 0, total_dt_number = 0, total_auv_number = 0, dt_total_length = 0, dt_total_area = 0, auv_total_length = 0, auv_total_area = 0, hov_total_length = 0, hov_total_area = 0;
			std::set<string>  setDTNumber;
			while (conn->next())
			{
				if (conn->value(0) == "DT")
				{
					dt_total_length += stof(conn->value(1));
					dt_total_area += stof(conn->value(2));
					
					std::string dtNumber = conn->value(3).substr(conn->value(4).length() + 1);
					int index = dtNumber.find("-");
					if (index == std::string::npos) dtNumber = dtNumber.substr(0, dtNumber.length() - 4);
					else dtNumber = dtNumber.substr(0, index);
					
					setDTNumber.insert(dtNumber);
				}
				else if (conn->value(0) == "AUV")
				{
					auv_total_length += stof(conn->value(1));
					//auv_total_area += stof(conn->value(2));
					total_auv_number++;
				}
				else if (conn->value(0) == "HOV")
				{
					hov_total_length += stof(conn->value(1));
					//hov_total_area += stof(conn->value(2));
					total_hov_number++;
				}
				
			}
			
			total_dt_number = setDTNumber.size();
			
			total_length = dt_total_length; // + auv_total_length + hov_total_length;
			total_area = dt_total_area; // + auv_total_area + hov_total_area;
			
			preface.append("{total_length: " + std::to_string(total_length) + 
							", total_area: " + std::to_string(total_area) + 
							", total_hov_number: " + std::to_string(total_hov_number) + 
							", total_dt_number: " + std::to_string(total_dt_number) + 
							", total_auv_number: " + std::to_string(total_auv_number) + 
							", dt_total_length: " + std::to_string(dt_total_length) + 
							", dt_total_area: " + std::to_string(dt_total_area) + 
							", auv_total_length: " + std::to_string(auv_total_length) + 
							", auv_total_area: " + std::to_string(auv_total_area) + 
							", hov_total_length: " + std::to_string(hov_total_length) + 
							", hov_total_area: " + std::to_string(hov_total_area) + "}}");
		}
		
		response.set_preface(preface);
    }
    
    // 查询分类信息
    if (requestStatistics.query_chart_data()) 
    {
	    std::string result = "{";
	    
	    {
			shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

			std::string sql = "select cruise_year, count(*) from t_source_data_side_scan where status_flag = 0 GROUP BY cruise_year;";
			
			PSS_LOGGER_DEBUG("query year book sql {0}", sql.data());
			
			status = conn->query(sql);
			
			if (status)
			{
				result.append("curise_year_data: [");
				int index = 0;
				while (conn->next())
				{
					if (index++ > 0) result.append(", ");
					result.append("{year: " +  conn->value(0) + ", number: " +  conn->value(1) +  "}");
				}
				result.append("]");
				
				// 有一个有效信息就算成功
				response.set_status(true);

			}
		}
		
		{
			shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

			std::string sql = "select priority, count(*) from t_source_data_side_scan where status_flag = 0 GROUP BY priority;";
			
			PSS_LOGGER_DEBUG("query year book sql {0}", sql.data());
			
			status = conn->query(sql);
			
			if (status)
			{
				if (result.length() > 1) result.append(", ");
				result.append("priority_data: [");
				int index = 0;
				while (conn->next())
				{
					if (index++ > 0) result.append(", ");
					std::string name = ("\"优先级 " + conn->value(0) + "\"");
					result.append("{name: " + name + ", value: " +  conn->value(1) +  "}");
				}
				result.append("]");
				
				// 有一个有效信息就算成功
				response.set_status(true);
			}
		}
		
		{
			shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();

			std::string sql = "select verify_flag, count(*) from t_source_data_side_scan where status_flag = 0 GROUP BY verify_flag;";
			
			PSS_LOGGER_DEBUG("query year book sql {0}", sql.data());
			
			status = conn->query(sql);
			
			if (status)
			{
				if (result.length() > 1) result.append(", ");
				result.append("verify_data: [");
				int index = 0;
				while (conn->next())
				{
					if (index++ > 0) result.append(", ");
					std::string name = (conn->value(0) == "0") ? "\"未查证\"" : "\"已查证\"";
					result.append("{name: " + name + ", value: " +  conn->value(1) +  "}");
				}
				result.append("]");
				
				// 有一个有效信息就算成功
				response.set_status(true);
			}
		}
		
		result.append("}");
		
		response.set_chart_data(result);
    }
    
	std::string pack = createPackage(CMD_QUERY_STATISTICS_DATA_BY_CONDITION_RESPONSE, response.SerializeAsString());
	sendAsyncPack(source.connect_id_, pack);
}

void CBaseCommand::sendAsyncPack(uint32_t id, const std::string& pack)
{
    if (pack.size() == 0) return;
    auto send_asyn_packet = std::make_shared<CMessage_Packet>();
    send_asyn_packet->buffer_.append(pack.c_str(), pack.size());
    session_service_->send_io_message(id, send_asyn_packet);
}

std::string CBaseCommand::generateCesiumPositionChain(const std::string &type)
{
	shared_ptr<MysqlConnection> conn = mMysqlConnectionPool->getConnection();
	std::string sql = "select cruise_number, type, name from t_source_data_cruise_route where type = \"" + type + "\" and status_flag = 0;";
	
	PSS_LOGGER_DEBUG("dt sql {0}", sql.data());
	
	bool status = conn->query(sql);
	
	if (!status) return "";

	std::vector<string> vectorCruiseNumbers;
	std::vector<string> vectorFileNames;
	while (conn->next())
	{
		vectorCruiseNumbers.push_back(conn->value(0));
		vectorFileNames.push_back(conn->value(2));
	}

	uint16_t size = vectorFileNames.size();
	std::string positionChains = "[[";
	for (uint16_t i = 0; i < size; i++)
	{
		std::string path = "/home/ftp_root/upload/" + vectorCruiseNumbers[i] + "/Navigation/" + type + "/" + vectorFileNames[i];
		std::ifstream fileDescripter(path);
		PSS_LOGGER_DEBUG("open file {0}", path);
		std::string position_chain = "";
		std::string line;
		uint64_t lineCount = 0;
		while(std::getline(fileDescripter, line))
		{
			if (lineCount++ % 5 != 0) continue;
			std::regex reg("^([0-9]+\\.[0-9]+)[ ]+([0-9]+\\.[0-9]+)[ ]+(.*)");
			std::smatch match;
			bool status = regex_search(line, match, reg);
			if (!status) continue;
			if (match.size() != 4) continue;
			
			if (position_chain.length() > 0) position_chain += ",";
			position_chain += std::string(match[1]) + "," + std::string(match[2]) + ",0";
		}
		if (position_chain.length() == 0) continue;
		if (positionChains.length() != 2) positionChains.append("],[");
		positionChains.append(position_chain);
	}
	positionChains.append("]]");
	
	return positionChains;
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
