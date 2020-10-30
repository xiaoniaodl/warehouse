var log4js = require('log4js');
log4js.configure({
    appenders: {
        out: { type: 'console' },
        allLog: { type: 'file', filename: '/var/log/warehouse.log' },
    },
    categories: {
        default: { appenders: ['out', 'allLog'], level: 'debug' },
        MQTT: { appenders: ['out', 'allLog'], level: 'debug' },
        express: { appenders: ['out', 'allLog'], level: 'debug' },
        "socket.io": { appenders: ['out', 'allLog'], level: 'debug' }
    }
});

exports.logger = function (category) {
    return log4js.getLogger(category);
};

exports.use = function (app) {
    app.use(log4js.connectLogger(log4js.getLogger('express'), { level: 'info', format: ':method :url' }));
}