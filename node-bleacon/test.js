var Bleacon = require('./index'),
    http = require('http');

Bleacon.startAdvertising('e2c56db5dffb48d2b060d0f5a71096e0', 0, 0, -59);

Bleacon.on('discover', function(bleacon) {
  console.log('bleacon found: ' + JSON.stringify(bleacon));

  var options = {
    host: 'thingworx-wvu.ptcmscloud.com',
    port: 80,
    path: '/Thingworx/Things/medisenseBLEscan/Properties/BLEdata?Accept=application/json&method=put&x-thingworx-session=true&value=' + JSON.stringify(bleacon) + '&appKey=314c5fc5-a06b-4fbb-b40a-4d87e24e9bff'
  };

  var req = http.request(options, function(resp){
    //console.log('STATUS: ' + resp.statusCode);
    //console.log('HEADERS: ' + JSON.stringify(resp.headers));
    resp.setEncoding('utf8');
    console.log("sent data");
    req.end();
  });
  req.on("error", function(e){
    console.log("Got error: " + e.message);
    //console.log(bleacon);
  });
  req.end();
});



Bleacon.startScanning(/*'e2c56db5dffb48d2b060d0f5a71096e0', 0, 0*/);
