var express = require('express'),
    app = express(),
    http = require('http'),
    socketIo = require('socket.io');

var lines = require('fs').readFileSync('roads.dat', 'utf-8')
    .split('\n')
    .filter(Boolean);

// Map for the roads, where the key is road_id
// This structure will connect roads to cars
road_map = {};
// simple 2D representation of roads for drawing purposes
var roads = [];

// roads_v2.dat
// roadID0 | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no |

// start webserver on port 8080
var server =  http.createServer(app);
var io = socketIo.listen(server);
server.listen(8080);
// add directory with our static files
app.use(express.static(__dirname + '/public'));
console.log("Server running on 127.0.0.1:8080");

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
