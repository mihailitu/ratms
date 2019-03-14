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
// simple 2D representation of roads for drawing
var roads = [];

// roads_v2.dat
// roadID0 | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no |
for(var i in lines) {
  console.log('Read: ' + lines[i])
  var road_data = lines[i].split(' ');
  // TODO: Normalize coordinates
  roads.push(
    {
        id: road_data[0],
        start: {
          x: road_data[5],
          y: road_data[6]
        },
        end: {
          x: road_data[7],
          y: road_data[8]
        }
    });

  road_map[road_data[0]] =
  {
    startGeo: {
      lat: road_data[1],
      lon: road_data[2]
    },
    endGeo: {
      lat: road_data[3],
      lon: road_data[4]
    },
    startCard: {
      x: road_data[5],
      y: road_data[6],
    },
    endCard: {
      x: road_data[7],
      y: road_data[8],
    },
    length: road_data[9],
    maxSpeed: road_data[10],
    lanes: road_data[11]
  };
}

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
  socket.emit('draw_map', {map: roads})
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
