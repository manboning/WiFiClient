#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

/*
 * 私密配置模板 —— 复制此文件为 wifi_config.h 并填入真实信息后即可编译
 * wifi_config.h 已被 .gitignore 排除，不会上传到 GitHub
 */

/* WiFi 热点 */
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"

/* 聚合数据天气 API（https://www.juhe.cn/docs/api/id/73 获取 key，city 参数为 URL 编码的城市名） */
#define WEATHER_URL   "http://apis.juhe.cn/simpleWeather/query?key=your-juhe-api-key&city=%E5%8C%97%E4%BA%AC"

#endif
