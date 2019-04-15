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
        id: Number(road_data[0]),
        lanes: Number(road_data[11]),
        start: {
            x: Number(road_data[5]),
            y: Number(road_data[6])
        },
        end: {
            x: Number(road_data[7]),
            y: Number(road_data[8])
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
        length: Number(road_data[9]),
        maxSpeed: Number(road_data[10]),
        lanes: Number(road_data[11])
    };
};

// console.log("Roads: " + JSON.stringify(roads, 0, 2));

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

function getCoordFromDist(startCoord, endCoord, distance) {
    // line lengths
    var xlen = endCoord.x - startCoord.x;
    var ylen = endCoord.y - startCoord.y;
    // hypotenuse length
    var hlen = Math.sqrt(Math.pow(xlen,2) + Math.pow(ylen,2));
    // Determine the ratio between they ditance value and the full hypotenuse.
    var ratio = distance / hlen;
    var newXLen = xlen * ratio;
    var newYLen = ylen * ratio;
    // The new X point is the starting x plus the smaller x length.
    var x = startCoord.x + newXLen;
    // Same goes for the new Y.
    var y = startCoord.y + newYLen;
    return {x, y};
}

function getCoordOnTheRoadLane(startCoord, endCoord, roadLane) {
    var angle = Math.atan2(endCoord.y - startCoord.y, endCoord.x - startCoord.x);
    var lanePix = roadLane * 10; // * pixels per lane
    // For driving on the left side of the road
    // var x = -Math.sin(angle) * lanePix + startCoord.x;
    // var y = Math.cos(angle) * lanePix + startCoord.y;
    // For driving on the right side of the road
    var x = Math.sin(angle) * lanePix + endCoord.x;
    var y = -Math.cos(angle) * lanePix + endCoord.y;
    return {x, y};
}

// entire vehicle data for each time frame
var frames = [];
var prevFrameTime = 0.0;

// all the roads involved in a timeframe
var roadsForTime = [];

for (var i in timeFrames) {
    var frameData = timeFrames[i].split(' ');
    var frameTime = frameData[0];
    var data = {
        roadID: Number(frameData[1]),
        trafficLights: [],
        vehicles: []
    };

    var lanes = frameData[2];
    var index = 3;
    for(var lane = 0; lane < lanes; ++lane) {
        var trafficLight = {
        }
        trafficLight.state = frameData[index++];
        switch(trafficLight.state) {
            case 'R':
                trafficLight.state = 'red';
                break;
            case 'G':
                trafficLight.state = 'green';
                break;
            case 'Y':
                trafficLight.state = 'yellow';
                break;
        }
        var coord = getCoordOnTheRoadLane(road_map[data.roadID].startCard, road_map[data.roadID].endCard, lane);
        trafficLight.x = coord.x/width;
        trafficLight.y = coord.y/height;
        data.trafficLights.push(trafficLight);
    }

    for (var v = index; v < frameData.length; v += 5) {
        //TODO: draw vehicle relative to the lane it's running on
        var vehicleCoord = getCoordFromDist(road_map[data.roadID].startCard, road_map[data.roadID].endCard, frameData[v]);
        vehicleCoord = getCoordOnTheRoadLane(road_map[data.roadID].startCard, vehicleCoord, frameData[v + 4]);
        var vehicle = {
            // TODO: calculate the exact position of the vehicle relative
            // to the road it belongs to
            x: vehicleCoord.x/width,
            y: vehicleCoord.y/height,
            d: frameData[v], // x: distance from the beginning of the road
            v: frameData[v + 1], // v: current velocity
            a: frameData[v + 2], // a: current acceleration
            id:frameData[v + 3], // id: vehicle id
            l: frameData[v + 4] //  l: the lane that this vehicle belongs to

        }
        data.vehicles.push(vehicle);
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
// console.log(JSON.stringify(frames, null, 2));
console.log("Frames: " + frames.length);
// console.log("Frames: " + JSON.stringify(frames, 0, 2));

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
    socket.emit('draw_state', {frameData: frames[frame++], frameNo: frame, frameTotal: frames.length});
    // add handler for message type "draw_line".
    socket.on('next_frame', function(data) {
        if (frame < frames.length) {
            socket.emit('draw_state', {frameData: frames[frame++], frameNo: frame, frameTotal: frames.length});
        } else {
            console.log("DONE");
            frame = 0;
            socket.emit('draw_state', {frameData: frames[frame++], frameNo: frame, frameTotal: frames.length});
        }
    });
});
