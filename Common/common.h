///
///
/// 控制系统命令定义
/// 命令头组成如下
/// Command ID (2 字节) + Package Length (2 字节) + IP (4 字节) + Parameter (任意长度)
///
///

#include <stdint.h>

// 数据库写入异常点记录 （任意字节数据内容）
const uint16_t CMD_INSERT_SIDE_SCAN_SOURCE_DATA = 0X7001;

// 数据库写入异常点记录 回复
const uint16_t CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE = 0XB001;

// 查询文件服务器开启状态
const uint16_t CMD_QUERY_FTP_SERVER_STATUS = 0X7003;

// 查询文件服务器开启状态回复
const uint16_t CMD_QUERY_FTP_SERVER_STATUS_RESPONSE = 0XB003;

// 数据库写入轨迹文件记录 （任意字节数据内容）
const uint16_t CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA = 0X7005;

// 数据库写入轨迹文件记录 回复
const uint16_t CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE = 0XB005;
