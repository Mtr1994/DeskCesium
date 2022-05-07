# DeskCesium
`QWebEngineView` + `Cesium` 实现的瓦片地图及一些基础操作



### 备注：

1. 该系统需要 `Cesium JS` 库，地址为： `https://cesium.com/platform/cesiumjs/`，也可以去 `github` 网站下载最新版本
2. 下载后，直接解压到 `./Resource/html` 目录下
3. 系统需要网络才能访问瓦片地图
3. 由于使用的是 `JavaScript` 语言，系统存在精度问题，在计算浮点数的时候，如果出现显示错误，可以修改 `Cesium.js` 代码，在数字变量后面加上 `.toFixed(6)`  代码
