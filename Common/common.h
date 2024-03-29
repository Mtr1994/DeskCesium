﻿///
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

// 查询可用检索条件
const uint16_t CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA = 0X7007;

// 查询可用检索条件 回复
const uint16_t CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE = 0XB007;

// 查询数据 条件查询
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER = 0X7009;

// 查询数据 回复
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER_RESPONSE = 0XB009;

// 查询数据 条件查询
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD = 0X700b;

// 查询数据 回复
const uint16_t CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD_RESPONSE = 0XB00b;

// 查询轨迹
const uint16_t CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE = 0X700d;

// 查询轨迹 回复
const uint16_t CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE = 0XB00d;

// 统计数据查询
const uint16_t CMD_QUERY_STATISTICS_DATA_BY_CONDITION = 0X700e;

// 统计数据查询回复
const uint16_t CMD_QUERY_STATISTICS_DATA_BY_CONDITION_RESPONSE = 0XB00e;
