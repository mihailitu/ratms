var express = require('express'),
    app = express(),
    http = require('http'),
    socketIo = require('socket.io');


// start webserver on port 8080
var server =  http.createServer(app);
var io = socketIo.listen(server);
server.listen(8080);
// add directory with our static files
app.use(express.static(__dirname + '/public'));
console.log("Server running on 127.0.0.1:8080");

// array of all lines drawn
var line_history = [];

var road = {
  id: false,
  startGeo: {
    lat: false,
    lon: false
  },
  endGeo: {
    lat: false,
    lon: false
  },
  startCart: {
    x: false,
    y: false
  },
  endCart: {
    x: false,
    y: false
  },
  length: 0,
  maxSpeex: 0,
  lanes_no: 0
}

const lineByLine = require('n-readlines');
var liner = new readlines('roads.dat');

// roads_v2.dat
// roadID0 | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no |

// event-handler for new incoming connections
io.on('connection', function (socket) {
  console.log("on connection: ");
   // first send the history to the new client
   // for (var i in line_history) {
   //   console.log("line_history X:" + line_history[i][0].x + " Y:" + line_history[i][0].y);
   //    socket.emit('draw_line', { line: line_history[i] } );
   // }



   // add handler for message type "draw_line".
   socket.on('draw_line', function (data) {
      // add received line to history
//      console.log("draw_line from: " + data.line[0].x + " " + data.line[0].y + " ");
//      console.log("draw_line to  : " + data.line[1].x + " " + data.line[1].y + " ");
//      line_history.push(data.line);
      // send line to all clients
      io.emit('draw_line', { line: data.line });
   });
});
