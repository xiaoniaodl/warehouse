const fs = require('fs')
const log = require('./log')
const mqttLog = log.logger('MQTT')
const expressLog = log.logger('express')
const mosca = require('mosca')

const express = require('express')
const bodyParser = require('body-parser')
const app = express()
const http = require('http').createServer(app)
const io = require('socket.io')(http)
const socketLog = log.logger('socket.io')
const port = 3030

var mqttAuth = new mosca.Authorizer()
var mqttServer = new mosca.Server({
    host: '0.0.0.0',
    port: 1883
})

// 传感器临界值
var CriticalValue = JSON.parse(fs.readFileSync('criticalValue.json').toString())

// 传感器数据模型
var warehouseData = [
    {
        id: '',
        mac: 'C8:2B:96:09:B8:F4',
        online: false,
        update_time: null,
        hum: 0,
        temp: 0,
        mq135: 0
    },
    {
        id: '',
        mac: 'A4:CF:12:DE:77:F0',
        online: false,
        update_time: null,
        hum: 0,
        temp: 0,
        mq135: 0
    },
    {
        id: '',
        mac: 'C8:2B:96:09:B7:46',
        online: false,
        update_time: null,
        hum: 0,
        temp: 0,
        mq135: 0
    },
    {
        id: '',
        mac: 'A4:CF:12:DE:BF:8B',
        online: false,
        update_time: null,
        hum: 0,
        temp: 0,
        mq135: 0
    },
]

log.use(app)
app.use(bodyParser.raw())
app.use(bodyParser.text())
app.use(bodyParser.json())
app.use(bodyParser.urlencoded({ extended: false }))

app.get('/metrics', (req, res) => {
    res.set('Content-Type', 'text/plain; version=0.0.4; charset=utf-8')
    res.set('Date', new Date().toUTCString())
    var text = ''
    for (var i = 0; i < warehouseData.length; i++) {
        var data = warehouseData[i]
        text += `# ${i + 1}号仓库数据\n`
        text += `warehouse_mqtt{id="${i + 1}",mac="${data.mac}",mqttid="${data.id}"} ${data.online ? '1' : '0'}\n`
        text += `warehouse_temp{id="${i + 1}"} ${data.temp}\n`
        text += `warehouse_hum{id="${i + 1}"} ${data.hum}\n`
        text += `warehouse_mq135{id="${i + 1}"} ${data.mq135}\n\n`
    }
    text += `# 传感器报警阈值\n`
    text += `warehouse_HUM_MIN ${CriticalValue.hum[0]}\n`
    text += `warehouse_HUM_MAX ${CriticalValue.hum[1]}\n`
    text += `warehouse_TEMP_MIN ${CriticalValue.temp[0]}\n`
    text += `warehouse_TEMP_MAX ${CriticalValue.temp[1]}\n`
    text += `warehouse_MQ135_MIN ${CriticalValue.mq135[0]}\n`
    text += `warehouse_MQ135_MAX ${CriticalValue.mq135[1]}\n`
    res.status(200).send(text)
})

app.use(express.static('www'))

io.on('connection', (socket) => {
    socketLog.info('客户端连接')
    socket.on('UpdateCriticalValue', (data) => {
        socketLog.info(`更新阈值 ${JSON.stringify(data)}`)
        for (var key in data) {
            CriticalValue[key] = data[key];
        }
        CriticalValue.mid = 0
        CriticalValue.mac = "null"
        mqttServer.publish({ topic: '/warehouse/CriticalValue', payload: JSON.stringify(CriticalValue) })
        fs.writeFileSync('criticalValue.json', JSON.stringify(CriticalValue))
        socket.emit('CriticalValue', { 'msg': 'ok' })
        socket.emit('CriticalValue', CriticalValue)
    })
    socket.emit('CriticalValue', CriticalValue)
    socket.emit('WarehouseData', warehouseData)
})

io.on('disconnect', (socket) => {
    socketLog.warn('客户端断开')
})

// 5秒内没上传数据视为离线
setInterval(() => {
    var time = new Date()
    for (var i = 0; i < warehouseData.length; i++) {
        if (warehouseData[i].online && (time - warehouseData[i].update_time) / 1000 > 5) {
            mqttLog.warn(`#${i + 1} 断开连接 ${warehouseData[i].id}`)
            warehouseData[i].online = false
            warehouseData[i].id = ''
            warehouseData[i].update_time = null
            warehouseData[i].hum = 0
            warehouseData[i].temp = 0
            warehouseData[i].mq135 = 0
        }
    }
    io.emit('WarehouseData', warehouseData)
}, 5000)


mqttServer.on('ready', function () {
    mqttServer.authenticate = mqttAuth.authenticate
    mqttServer.authorizePublish = mqttAuth.authorizePublish
    mqttServer.authorizeSubscribe = mqttAuth.authorizeSubscribe

    mqttAuth.addUser('iot', '******', '/warehouse/data', '/warehouse/CriticalValue', function (error) {
        if (error)
            mqttLog.error("添加MQTT用户失败")
    })
})

mqttServer.on('published', function (packet, client) {
    if (packet.topic == "/warehouse/data") {
        var obj = JSON.parse(packet.payload.toString())
        for (var i = 0; i < warehouseData.length; i++) {
            if (obj['mac'] != undefined && obj['mac'] == warehouseData[i].mac) {
                obj['id'] = i + 1
                CriticalValue.mid = i + 1
                CriticalValue.mac = obj['mac']
                mqttServer.publish({ topic: '/warehouse/CriticalValue', payload: JSON.stringify(CriticalValue) })
            }
            if (obj['id'] != undefined && obj['id'] == (i + 1)) {
                warehouseData[i].id = client.id
                warehouseData[i].online = true
                warehouseData[i].update_time = new Date()
                if (obj['hum'] != undefined) warehouseData[i].hum = obj['hum']
                if (obj['temp'] != undefined) warehouseData[i].temp = obj['temp']
                if (obj['mq135'] != undefined) warehouseData[i].mq135 = obj['mq135']
            }
        }
    }
    mqttLog.info(`消息发布(${packet.topic}) ${packet.payload.toString()}`)
    io.emit('WarehouseData', warehouseData)
})

mqttServer.on('subscribed', function (topic, client) {
    mqttLog.info(`客户端订阅 ${client.id} ${topic}`)
})

mqttServer.on('unSubscribed', function (topic, client) {
    mqttLog.info(`取消订阅 ${client.id} ${tpoic}`)
})

mqttServer.on('clientConnected', function (client) {
    mqttLog.info(`客户端连接 ${client.id}`)
})

mqttServer.on('clientDisConnected', function (client) {
    for (var i = 0; i < warehouseData.length; i++) {
        if (client.id == warehouseData[i].id) {
            warehouseData[i].id = ''
            warehouseData[i].online = false
            warehouseData[i].update_time = null
            mqttLog.warn(`#${i + 1} 断开连接 ${client.id}`)
        }
    }
})

http.listen(port, () => expressLog.log(`App listening on port ${port}!`))