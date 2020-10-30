# 仓库环境监测平台

## 硬件部分

* 主控：NodeMCU
NodeMCU是一个开源的物联网平台，底层使用ESP8266。
* 温湿度传感器：DHT-11
DHT11是一款有已校准数字信号输出的温湿度传感器。 其精度湿度+-5%RH， 温度+-2℃，量程湿度20-90%RH， 温度0~50℃。
* 空气污染传感器：MQ-135
MQ-135气体传感器对氨气、硫化物、苯系蒸汽的灵敏度高，对烟雾和其它有害气体的监测也很理想。这种传感器可检测多种有害气体，是一款适合多种应用的低成本传感器。
* 无源蜂鸣器
* LCD 1602（配IIC总线转换器）

## 通信协议：MQTT
MQTT是一个基于客户端-服务器的消息发布/订阅传输协议。MQTT协议是轻量、简单、开放和易于实现的，这些特点使它适用范围非常广泛。

## 软件工作平台：Docker
Docker 是一个开源的应用容器引擎，让开发者可以打包他们的应用以及依赖包到一个可移植的容器中,然后发布到任何流行的Linux机器或Windows 机器上,也可以实现虚拟化,容器是完全使用沙箱机制,相互之间不会有任何接口。

## 软件部署工具：Docker Compose
Docker Compose是 docker 提供的一个命令行工具，用来定义和运行由多个容器组成的应用。使用 compose，可以通过 YAML 文件声明式的定义应用程序的各个服务，并由单个命令完成应用的创建和启动。

## 软件功能模块

模块|说明
--|:--:
node-exporter | 开源项目，用 Go 语言编写的系统指标收集器，用于机器系统数据收集，如：CPU、内存、磁盘、I/O等信息。
Prometheus | Prometheus 是一个开源的服务监控系统和时间序列数据库。
Loki | 开源项目，是一个水平可扩展，高可用性，多租户的日志聚合系统。
promtail | 日志抓取工具，负责监听日志文件并写入到Loki。
Grafana | Grafana 是一款采用 Go 语言编写的开源应用，主要用于大规模指标数据的可视化展现，是网络架构和应用分析中最流行的时序数据展示工具，目前已经支持绝大部分常用的时序数据库。
warehouse-service | 用 NodeJs 语言编写的 MQTT 服务器，负责连接硬件，接收传感器数据，并提供支持 Prometheus 抓取的数据格式。

## 主要文件

文件|说明
--|:--:
NodeMCU.ino | NodeMCU 硬件部分程序，负责连接MQTT服务器并上报传感器数据，以及异常报警。
DHT.cpp / DHT.h | DHT-11 温湿度传感器库
LiquidCrystal_I2C.cpp / LiquidCrystal_I2C.h | 适用于IIC接口的LCD显示屏库
index.js | warehouse-service 源码，MQTT服务器程序，负责连接硬件并接收传感器数据。
log.js | 日志输出库，提供整齐的日志输出。
run.sh | warehouse-service 服务运行脚本，可在MQTT服务器运行前初始化 Grafana。
Dockerfile | 将用NodeJs编写的MQTT服务器程序打包成Docker镜像。
docker-compose.yaml | 定义构成应用程序的服务
loki-local-config.yaml | Loki 配置文件
promtail-docker-config.yaml | promtail 配置文件
docker-compose | 适用于x64架构的 docker-compose 可执行文件

## 部署流程

### 硬件部署流程

1. 下载Arduino IDE开发环境 [exe安装包](https://www.arduino.cc/download_handler.php?f=/arduino-1.8.12-windows.exe)
2. 添加 ESP8266 支持
   1. 安装完成以后，进入首选项（Preferences），找到附加开发板管理器地址（Additional Board Manager URLs），并在其后添加如下信息：
http://arduino.esp8266.com/stable/package_esp8266com_index.json
   2. 之后点击工具 - 开发板 - 开发板管理器，进入开发板管理器界面，找到esp8266并安装。
   3. 安装完成后，重启 Arduino IDE 软件。在工具 - 开发板选项中即会看到 ESP8266 开发板的选项。
3. 用Arduino IDE 软件打开 NodeMCU.ino。
4. 插上硬件，编译并上传程序。

### 软件部署流程（以Centos7为例）
```(bash)
# 修改yum源为阿里云源
wget -O /etc/yum.repos.d/CentOS-Base.repo http://mirrors.aliyun.com/repo/Centos-7.repo
yum makecache
# 安装所需的软件包
yum install -y yum-utils device-mapper-persistent-data lvm2
# 设置docker仓库
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
# 安装 Docker Engine-Community
yum install -y docker-ce docker-ce-cli containerd.io
# 配置镜像加速器
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json <<-'EOF'
{
  "registry-mirrors": ["https://3m7egqv1.mirror.aliyuncs.com"]
}
EOF
sudo systemctl daemon-reload
sudo systemctl restart docker
# 运行Warehouse服务
cd ./WarehouseService
../docker-compose up --build
# 查看服务运行状态
../docker-compose ps

                Name                              Command               State                Ports
----------------------------------------------------------------------------------------------------------------
warehouseservice_grafana_1             /run.sh                          Up      0.0.0.0:3000->3000/tcp
warehouseservice_loki_1                /usr/bin/loki -config.file ...   Up      3100/tcp
warehouseservice_node-exporter_1       /bin/node_exporter               Up      9100/tcp
warehouseservice_prometheus_1          /bin/prometheus --config.f ...   Up      9090/tcp
warehouseservice_promtail_1            /usr/bin/promtail -config. ...   Up
warehouseservice_warehouse-service_1   bash run.sh                      Up      0.0.0.0:1883->1883/tcp, 3030/tcp
```
