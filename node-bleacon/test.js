var Bleacon = require('./index'),
    http = require('http')
var array = [];

Bleacon.startAdvertising('e2c56db5dffb48d2b060d0f5a71096e0', 0, 0, -59);

Bleacon.on('discover', function(bleacon) {
  console.log('bleacon found: ' + JSON.stringify(bleacon));

  var options = {
    host: 'thingworx-wvu.ptcmscloud.com',
    port: 80,
    method: 'POST',
    path: '/Thingworx/Things/medisenseBLEscan/Properties/BLEdata?Accept=application/json&method=post&x-thingworx-session=true&value={"dataShape":{"fieldDefinitions":{"uuid":{"baseType":"STRING","description":"uuid","name":"uuid","aspects":{},"ordinal":0},"major":{"baseType":"NUMBER","description":"major","name":"major","aspects":{},"ordinal":0},"minor":{"baseType":"NUMBER","description":"minor","name":"minor","aspects":{},"ordinal":0},"measuredPower":{"baseType":"NUMBER","description":"measuredPower","name":"measuredPower","aspects":{},"ordinal":0},"rssi":{"baseType":"NUMBER","description":"rssi","name":"rssi","aspects":{},"ordinal":0},"accuracy":{"baseType":"NUMBER","description":"accuracy","name":"accuracy","aspects":{},"ordinal":0},"proximity":{"baseType":"STRING","description":"uuid","name":"uuid","aspects":{},"ordinal":0}}},"rows":[' + JSON.stringify(bleacon) + ']}&appKey=314c5fc5-a06b-4fbb-b40a-4d87e24e9bff'
};

  var req = http.request(options, function(resp){
    //console.log('STATUS: ' + resp.statusCode);
    //console.log('HEADERS: ' + JSON.stringify(resp.headers));
    resp.setEncoding('utf8');
    console.log("sent data");
    //req.end();
  });
  req.on("error", function(e){
    console.log("Got error: " + e.message);
    //console.log(bleacon);
  });
  req.end();


});

Bleacon.startScanning(/*'e2c56db5dffb48d2b060d0f5a71096e0', 0, 0*/);
