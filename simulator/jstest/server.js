var express = require('express'),
    app = express(),
    http = require('http'),
    socketIo = require('socket.io');

var lines = require('fs').readFileSync('roads.dat', 'utf-8')
    .split('\n')
    .filter(Boolean);

// Map of the roads, where the key is road_id
// This structure will hold all road info and it will connect cars to roads
var road_map = {};

// simple 2D representation of roads for drawing in client
var roads = [];

// roads_v2.dat
// roadID0 | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no |
for (var i in lines) {
    var road_data = lines[i].split(' ');

    roads.push({
        id: road_data[0],
        lanes: road_data[11],
        start: {
            x: road_data[5],
            y: road_data[6]
        },
        end: {
            x: road_data[7],
            y: road_data[8]
        }
    });

    road_map[road_data[0]] = {
        startGeo: {
            lat: road_data[1],
            lon: road_data[2]
        },
        endGeo: {
            lat: road_data[3],
            lon: road_data[4]
        },
        startCard: {
            x: Number(road_data[5]),
            y: Number(road_data[6]),
        },
        endCard: {
            x: Number(road_data[7]),
            y: Number(road_data[8]),
        },
        length: road_data[9],
        maxSpeed: road_data[10],
        lanes: road_data[11]
    };
};

// find out the dimensions of the map to normalize coordinates
width = Math.max.apply(Math, roads.map(
    function(o) {
        return o.end.x;
    }));

height = Math.max.apply(Math, roads.map(
    function(o) {
        return o.end.y;
    }));

console.log('Map size: [' + width + ', ' + height + ']');

// normalize coordinates
roads.forEach(function(road) {
    road.start.x = road.start.x / width;
    road.start.y = road.start.y / height;
    road.end.x = road.end.x / width;
    road.end.y = road.end.y / height;
});

// read vehicle position
var timeFrames = require('fs').readFileSync('output.dat', 'utf-8')
    .split('\n')
    .filter(Boolean);

function getCoordFromDist__(x1, y1, x2, y2, distance) {
    // Determine line lengths
    var xlen = x2 - x1;
    var ylen = y2 - y1;

    // Determine hypotenuse length
    var hlen = Math.sqrt(Math.pow(xlen,2) + Math.pow(ylen,2));

    // The variable identifying the length of the `shortened` line.
    // In this case 50 units.
    var smallerLen = distance;

    // Determine the ratio between they shortened value and the full hypotenuse.
    var ratio = smallerLen / hlen;

    var smallerXLen = xlen * ratio;
    var smallerYLen = ylen * ratio;

    // The new X point is the starting x plus the smaller x length.
    var smallerX = x1 + smallerXLen;

    // Same goes for the new Y.
    var smallerY = y1 + smallerYLen;
    return {x: smallerX, y: smallerY};
}

function getCoordFromDist(startCoord, endCoord, distance) {
    console.log({startCoord, endCoord});
    // line lengths
    var xlen = endCoord.x - startCoord.x;
    var ylen = endCoord.y - startCoord.y;
    console.log({xlen, ylen});
    // hypotenuse length
    var hlen = Math.sqrt(Math.pow(xlen,2) + Math.pow(ylen,2));
    console.log({hlen});
    // Determine the ratio between they ditance value and the full hypotenuse.
    var ratio = distance / hlen;
    console.log({ratio});
    var newXLen = xlen * ratio;
    var newYLen = ylen * ratio;
    console.log({newXLen, newYLen});
    // The new X point is the starting x plus the smaller x length.
    var x = startCoord.x + newXLen;
    // Same goes for the new Y.
    var y = startCoord.y + newYLen;
    console.log(startCoord.y + ' + ' + newYLen + ' = ' + y);
    console.log({x: x, y: y});
    return {x, y};
}

// each time frame
var frames = [];
var prevFrameTime = 0.0;

// all the roads involved in a timeframe
var roadsForTime = [];

for (var i in timeFrames) {
    var frameData = timeFrames[i].split(' ');
    var frameTime = frameData[0];
    var data = {
        roadID: frameData[1],
        vehicles: []
    };

    for (var v = 2; v < frameData.length; v += 4) {
        console.log("Road: [" + JSON.stringify(road_map[data.roadID].startCard) + ', ' + JSON.stringify(road_map[data.roadID].endCard) + ']');
        var vehicleCoord = getCoordFromDist(road_map[data.roadID].startCard, road_map[data.roadID].endCard, frameData[v]);
        // var vehicleCoord = getCoordFromDist(road_map[data.roadID].startCard.x, road_map[data.roadID].startCard.y,
        //                                     road_map[data.roadID].endCard.x, road_map[data.roadID].endCard.y,
        //                                     frameData[v]);
        console.log("VCoord: [" + JSON.stringify(vehicleCoord) + ']');
        data.vehicles.push({
            // TODO: calculate the exact position of the vehicle relative
            // to the road it belongs to
            x: vehicleCoord.x/width,
            y: vehicleCoord.y/height,
            d: frameData[v], // x: distance from the beginning of the road
            v: frameData[v + 1], // v: current velocity
            a: frameData[v + 2], // a: current acceleration
            l: frameData[v + 3] //  l: the lane that this vehicle belongs to
        });
        console.log(JSON.stringify(data, null, 2));
    };

    if (prevFrameTime != frameTime) {
        frames.push({
            time: prevFrameTime,
            data: roadsForTime,
        });
        roadsForTime = [];
        prevFrameTime = frameTime;
    }

    roadsForTime.push(data);
}
console.log(JSON.stringify(frames, null, 2));

// start webserver on port 8080
var server = http.createServer(app);
var io = socketIo.listen(server);
server.listen(8080);

// add directory with our static files
app.use(express.static(__dirname + '/public'));
console.log("Server running on 127.0.0.1:8080");

// event-handler for new incoming connections
io.on('connection', function(socket) {
    console.log("on connection: ");

    socket.emit('draw_map', {
        map: roads
    });

    var frame = 0;
    // send vehicle status
    socket.emit('draw_state', frames[frame++]);
    // add handler for message type "draw_line".
    socket.on('next_frame', function(data) {
        if (frame < frames.length) {
            socket.emit('draw_state', frames[frame++]);
        } else {
            console.log("DONE");
        }
    });
});
