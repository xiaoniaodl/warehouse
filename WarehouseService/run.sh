if [ $GRAFANA_HOST ]; then
    GRAFANA_HOST="http://admin:admin@${GRAFANA_HOST}"
else
    GRAFANA_HOST="http://admin:admin@slave1.zuozishi.info:3000"
fi

if [ $GRAFANA_PASSWORD ]; then
    GRAFANA_PASSWORD="******"
fi

echo "* 检测连通性"
res_code=`curl -I -m 10 -o /dev/null -s -w %{http_code} ${GRAFANA_HOST}`
while [ $res_code != "200" ]
do
    echo "[`date +%F` `date +%T`] Grafana未启动"
    sleep 1
    res_code=`curl -I -m 10 -o /dev/null -s -w %{http_code} ${GRAFANA_HOST}`
done
echo "[`date +%F` `date +%T`] Grafana已启动"

echo "* 默认密码检测"
res_code=`curl -I -m 10 -o /dev/null -s -w %{http_code} ${GRAFANA_HOST}/api/datasources`
if [ $res_code != "200" ]; then
    echo "- 非默认密码，跳过初始化"
    echo "* 运行Warehouse Service"
    npm run start
    exit
fi
echo "- 默认密码，开始初始化"

echo "* 创建数据源 - Prometheus"
curl "${GRAFANA_HOST}/api/datasources" -X POST \
-H "Content-Type:application/json" \
-d '{"name":"Prometheus","type":"prometheus","access":"proxy","url":"http://prometheus:9090","basicAuth":false,"isDefault":true}'
echo ''

echo "* 创建数据源 - Loki"
curl "${GRAFANA_HOST}/api/datasources" -X POST \
-H "Content-Type:application/json" \
-d '{"name":"Loki","type":"loki","access":"proxy","url":"http://loki:3100","basicAuth":false,"isDefault":false}'
echo ''

for file in `ls ./dashboards/*.json`
do
    echo "* 创建仪表盘 - $file"
    cat $file > temp.json
    node ./init-dashboard-json.js

    curl "${GRAFANA_HOST}/api/dashboards/db" -X POST \
    -H "Content-Type:application/json" \
    -d @temp.json
    echo ''
done

rm -f temp.json

echo "* 运行Warehouse Service"
npm run start