# 指定使用最新版本的NodeJs环境
FROM node:latest

# 建立工作目录
RUN mkdir /app
WORKDIR /app

# 复制文件到镜像
COPY . /app

# 安装依赖
RUN npm install --registry=https://registry.npm.taobao.org

# MQTT服务端口
EXPOSE 1883
# Web端口
EXPOSE 3030

ENV GRAFANA_HOST grafana:3000
ENV GRAFANA_PASSWORD ******

ENTRYPOINT ["bash","run.sh"]