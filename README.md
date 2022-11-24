# DeskCesium
`QWebEngineView` + `Cesium` 实现的瓦片地图及一些基础操作



### 备注：

1. 该系统需要 `Cesium JS` 库，地址为： `https://cesium.com/platform/cesiumjs/`，也可以去 `github` 网站下载最新版本
2. 下载后，直接解压到 `./Resource/html` 目录下
3. 系统需要网络才能访问瓦片地图
3. 由于使用的是 `JavaScript` 语言，系统存在精度问题，在计算浮点数的时候，如果出现显示错误，可以修改 `Cesium.js` 代码，在数字变量后面加上 `.toFixed(6)`  代码



# `DeskCesium` 研发技术栈

#### 一、表格解析

* 下载 `QXlsx` 开源库，使用 `Qt 5.15.2` 版本直接打开 `QXlsx` 文件夹下的工程文件，编译生成静态库文件。
* 注意：由公式得出的空单元格，内容为空，但其实不是真的空，会存在一个空字符串，导致解析多出几行或几列。

* 注意：获取图像接口 `bool getImage(int row, int col, QImage& img);` 行、列序号都是从 0 开始数的。

#### 二、数据库配置

* 查看流程：`https://mtr1994.github.io/blog/2022-07/t1`

* 数据表结构请查看附件 `数据库设计.md`

* 数据库外部连接库安装：`sudo apt-get install libmysqlclient-dev`

  ```
  数据库因为事务未提交被锁住问题
  1、查看当前线程
  	show full processlist;
  2、查看事务表
  	select * from information_schema.innodb_trx;
  3、在数据库连接中执行 kill trx_mysql_thread_id ，参数 id 是第二条语句查询的
  
  注意事项：
  	rollback 执行后， 事务默认自动关闭
  ```

#### 三、文件服务器配置 （`vsftpd`）

* 安装：`sudo apt install vsftpd`，安装完成后，配置文件在 `/etc/vsftpd.conf`

* 查看运行状态：`service vsftpd status`

* 关闭服务：`service vsftpd stop`

* 重启服务：`systemctl restart vsftpd.service`

* 具体配置参考：`https://www.cnblogs.com/yanghailin888/articles/10329246.html`

* ```
  # 修改用户权限，只能用于登录 ftp
  sudo cat /etc/passwd
  sudo useradd -d /home/ftp_root idsse
  sudo passwd idsse
  sudo usermod -s /sbin/nologin idsse
  
  # 修改 FTP 用户可登录
   sudo vim /etc/pam.d/vsftpd （修改最后一句中的 auth required pam_shells.so 为 auth required pam_nologin.so）
  
  # 限制数据传输端口
  sudo vim /etc/vsftpd.conf  (pasv_enable=YES pasv_min_port=50000 pasv_max_port=55000)
  
  # 打开写入权限
  sudo vim /etc/vsftpd.conf  (write_enable=YES)
  
  # 修改上传的文件权限
  sudo vim /etc/vsftpd.conf  (local_umask=022)
  ```

* 修改根目录的拥有者和组：`sudo chown -R idsse:idsse ./ftp_root/`，之后新建的文件的文件夹，用户 `idsse` 都能拥有写权限

#### 四、后台服务框架编译

* 框架地址：·https://github.com/OpenArkStudio/PSS_ASIO/releases/tag/v3.0.0·
* 编译；下载加压，使用 `CMake` 生成项目，在 `Build` 文件夹中运行 `sudo make -j4`
* 生成的可执行程序及依赖库存在于 `Build->Linux` 下，赋予主程序可运行权限 `sudo chmod 777 ./pss_asio`
* 配置主程序端口，运行
* 添加特有命令解析程序
* 查看是否是 `Release` 编译 `readelf -S deskcesium | grep debug`
* 后台服务配置：拷贝编译后的程序、库及配置文件到服务器；添加运行库检索地址 `sudo vim ld.so.conf` ，添加程序所在目录到最后一行，保存，运行 `sudo ldconfig`

#### 五、序列化

* 下载程序：`https://github.com/protocolbuffers/protobuf/releases/download/v21.9/protoc-21.9-win64.zip`，用于序列化结构体
* 语法格式：`https://developers.google.cn/protocol-buffers/docs/proto3`
* 编写 `proto` 结构体文件：`protoc --cpp_out=./ .\source.proto` (处理当前文件夹下的 `source.proto` 文件， 生成的结果也放在当前文件夹下)
* 修改：生成的头文件中加入定义 `#define PROTOBUF_USE_DLLS ` 或者在 `pro` 文件中加入 `DEFINES += PROTOBUF_USE_DLLS`，清理后编译
* 源码编译：`https://github.com/protocolbuffers/protobuf/releases/download/v21.9/protobuf-cpp-3.21.9.zip`
* 编译完成后，通过 `CMake` 引用时，需要将生成的 `.cc 和 .h` 文件加入 `source_group` 中，否则会导致消息类找不到
* 引用的库是 `libprotobuf.so` 动态库，生成的 `libprotobuf-lite` 是需要通过 `protoc` 运行参数配置的，用于程序运行时依赖,而不是编译时

#### 六、数据库连接池

* 使用第三方开源代码实现：待定
* 数据库连接初始化可以在模块加载时进行
* 数据库需要支持事务功能，方便回滚

#### 七、资源服务 `Nginx` 配置

* 安装：sudo apt-get install nginx

* 启动：sudo service nginx start

* 查看运行状态：service nginx status

* 访问：在浏览器地址栏输入服务器地址，出现欢迎页面就表示网站服务启动成功

* 配置图片访问服务：`https://www.cnblogs.com/youran-he/p/15523071.html`

* 重启服务：`nginx -s reload`

* 关闭服务：`nginx -s stop`


#### 八、技术点

* `ScreenOverlay` 不支持 TIFF 图片展示，显示在屏幕上
* `GroundOverlay` 显示在地球上的贴图
* `贴 TIFF` 图片，`GDAL` 可以打开直接服务器图片进行计算，前端也会直接加载远程图片资源显示
