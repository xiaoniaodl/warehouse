const fs = require('fs')

var output = {
    "dashboard": {},
    "folderId": 0,
    "overwrite": false
}

jsonBuff = fs.readFileSync('temp.json')
output.dashboard = JSON.parse(jsonBuff.toString())
output.dashboard.id = null
output.dashboard.uid = null
fs.writeFileSync('temp.json',JSON.stringify(output))