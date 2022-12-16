// DeskCesium_Logic.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "IFrameObject.h"
#include "define.h"

#include <vector>

#include "BaseCommand.h"

#if PSS_PLATFORM == PLATFORM_WIN
#ifdef DESKCESIUM_LOGIC_EXPORTS
#define DECLDIR extern "C" _declspec(dllexport)
#else
#define DECLDIR extern "C"__declspec(dllimport)
#endif
#else
#define DECLDIR extern "C"
#endif

using namespace std;

DECLDIR int load_module(IFrame_Object* frame_object, string module_param);
DECLDIR void unload_module();
DECLDIR int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
DECLDIR int module_state();
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);

ISessionService* session_service = nullptr;
std::shared_ptr<CBaseCommand> base_command = nullptr;

#define MESSAGE_FUNCTION_BEGIN(x) switch(x) {
#define MESSAGE_FUNCTION(x,y,z,h,i) case x: { y(z,h,i); break; }
#define MESSAGE_FUNCTION_END }

//插件加载
int load_module(IFrame_Object* frame_object, string module_param)
{
#ifdef GCOV_TEST
    //如果是功能代码覆盖率检查，则开启这个开关，让插件执行所有框架接口调用
    PSS_LOGGER_DEBUG("[load_module]gcov_check is set.");
#endif

    //初始化消息处理类
    base_command = std::make_shared<CBaseCommand>();

    //注册插件
    frame_object->Regedit_command(LOGIC_COMMAND_CONNECT);
    frame_object->Regedit_command(LOGIC_COMMAND_DISCONNECT);
	
	// 自定义命令
    frame_object->Regedit_command(CMD_INSERT_SIDE_SCAN_SOURCE_DATA);
    frame_object->Regedit_command(CMD_QUERY_FTP_SERVER_STATUS);
    frame_object->Regedit_command(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA);
    frame_object->Regedit_command(CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA);
    frame_object->Regedit_command(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER);
	frame_object->Regedit_command(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD);
	
    session_service = frame_object->get_session_service();

    base_command->Init(session_service);

    PSS_LOGGER_DEBUG("[load_module]({0})finish.", module_param);

    return 0;
}

//卸载插件
void unload_module()
{
    PSS_LOGGER_DEBUG("[unload_module]finish.");
}

//执行消息处理
int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //插件消息处理
    //PSS_LOGGER_DEBUG("[do_module_message]command_id={0}.", recv_packet.command_id_);

    MESSAGE_FUNCTION_BEGIN(recv_packet->command_id_);
    MESSAGE_FUNCTION(LOGIC_COMMAND_CONNECT, base_command->logic_connect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_COMMAND_DISCONNECT, base_command->logic_disconnect, source, recv_packet, send_packet);
    
    MESSAGE_FUNCTION(CMD_INSERT_SIDE_SCAN_SOURCE_DATA, base_command->logic_insert_side_scan_source_data, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(CMD_QUERY_FTP_SERVER_STATUS, base_command->logic_query_ftp_server_status, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA, base_command->logic_insert_cruise_route_source_data, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA, base_command->logic_query_search_filter_parameter_data, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER, base_command->logic_query_side_scan_source_data_by_filter, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD, base_command->logic_query_side_scan_source_data_by_keyword, source, recv_packet, send_packet);

    MESSAGE_FUNCTION_END;

    return 0;
}

//获得当前插件状态
int module_state()
{
    return 0;
}

//设置日志输出
void set_output(shared_ptr<spdlog::logger> logger)
{
    //设置输出对象
    spdlog::set_default_logger(logger);
}

