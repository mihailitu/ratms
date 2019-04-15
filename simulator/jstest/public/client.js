document.addEventListener("DOMContentLoaded", function() {
    var mouse = {
        click: false,
        move: false,
        pos: {x:0, y:0},
        pos_prev: false
    };
    // get canvas element and create context
    document.title = "Bla bla";
    var canvas  = document.getElementById('drawing');
    var context = canvas.getContext('2d');
    var viewWidth   = window.innerWidth - 10;
    var viewHeight  = window.innerHeight - 10;
    var socket  = io.connect();

    // set canvas to full browser width/height
    canvas.width = window.innerWidth - 10;
    canvas.height = window.innerHeight - 10;

    console.log("client canvas[" + canvas.width + ", " + canvas.height + "]");
    console.log("client view[" + viewWidth + ", " + viewHeight + "]");
    // register mouse event handlers
    canvas.onmousedown = function(e){ mouse.click = true; };
    canvas.onmouseup = function(e){ mouse.click = false; };

    canvas.onmousemove = function(e) {
        // normalize mouse position to range 0.0 - 1.0
        mouse.pos.x = e.clientX / viewWidth;
        mouse.pos.y = e.clientY / viewHeight;
        mouse.move = true;
    };

    function sleep(start, delay) {
        // var start = new Date().getTime();
        while (new Date().getTime() < start + delay);
    }
    // save road map. It will not change through the run of the program
    road_map = {};

    // draw line received from server
    socket.on('draw_map', function (data) {
        road_map = data.map;
        console.log("Map loaded. Length: " + road_map.length)
    });

    var delay = 500; //ms
    var statFrameTime = new Date().getTime();

    var vehicleColors = [//"AliceBlue", "AntiqueWhite", "Aqua", "Aquamarine", "Azure", "Beige", "Bisque",
    "Black", "BlanchedAlmond", "Blue", "Brown", "BurlyWood", "CadetBlue", "Chartreuse", "Chocolate", "Coral",
    "CornflowerBlue", "Cornsilk", "Crimson", "Cyan", "DarkBlue", "DarkCyan", "DarkGoldenRod", "DarkGray", "DarkGrey",
    "DarkGreen", "DarkKhaki", "DarkMagenta", "DarkOliveGreen", "DarkOrange", "DarkOrchid", "DarkRed", "DarkSalmon",
    "DarkSeaGreen", "DarkSlateBlue", "DarkSlateGray", "DarkSlateGrey", "DarkTurquoise", "DarkViolet", "DeepPink",
    "DeepSkyBlue", "DimGray", "DimGrey", "DodgerBlue", "FireBrick", "FloralWhite", "ForestGreen", "Fuchsia",
    "Gainsboro", "GhostWhite", "Gold", "GoldenRod", "Gray", "Grey", "Green", "GreenYellow", "HoneyDew", "HotPink",
    "IndianRed", "Indigo", "Ivory", "Khaki", "Lavender", "LavenderBlush"]

    socket.on('draw_state', function (data) {
        var canvas  = document.getElementById('drawing');
        var context = canvas.getContext('2d');

        // clear canvas
        context.clearRect(0, 0, viewWidth, viewHeight);

        var frameTxt = "Time: " + data.frameData.time;
        var textX = viewWidth - context.measureText("Time: 000000.000000").width;
        var textY = 15;
        var textH = 15;
        context.fillStyle = 'black';
        context.fillText(frameTxt, textX, textY);
        textY += textH;
        context.fillText("Frame: " + data.frameNo + " / " + data.frameTotal, textX, textY);
        textY += textH;
        context.fillText("Roads: " + road_map.length, textX, textY);
        textY += textH;
        var totalFramevehiclesNo = 0;
        for(var i = 0; i < data.frameData.data.length; ++i)
            totalFramevehiclesNo += data.frameData.data[i].vehicles.length;
        context.fillText("Vehicles: " + totalFramevehiclesNo, textX, textY);

        // first, draw the roads
        for(var i = 0; i < road_map.length; i++) {
            var road = road_map[i];
            // console.log('drawing: ' + road.id + ': ' + JSON.stringify(road));
            context.beginPath();
            context.lineWidth = road.lanes * 5;
            context.moveTo(road.start.x * viewWidth, road.start.y * viewHeight);
            context.lineTo(road.end.x * viewWidth, road.end.y * viewHeight);
            context.strokeStyle = "#eff2f7";
            // context.strokeStyle = 'green';
            context.stroke();
        }

        for(var i = 0; i < data.frameData.data.length; ++i) {
            // Traffic lights
            for(var l = 0; l < data.frameData.data[i].trafficLights.length; ++l) {
                context.beginPath();
                context.arc(data.frameData.data[i].trafficLights[l].x * viewWidth, data.frameData.data[i].trafficLights[l].y * viewHeight, 3, 0, 2 * Math.PI);
                context.fillStyle = data.frameData.data[i].trafficLights[l].state;
                context.lineWidth = 1;
                context.stroke();
                context.fill();
            }

            // Vehicles
            for(var v = 0; v < data.frameData.data[i].vehicles.length; ++v) {
                context.beginPath();
                context.arc(data.frameData.data[i].vehicles[v].x * viewWidth, data.frameData.data[i].vehicles[v].y * viewHeight, 3, 0, 2 * Math.PI);
                context.fillStyle = vehicleColors[data.frameData.data[i].vehicles[v].id];
                context.strokeStyle = "red";
                context.lineWidth = 1;
                context.stroke();
                context.fill();
            }
        }

        sleep(statFrameTime, delay);
        statFrameTime = new Date().getTime();
        socket.emit('next_frame');
    });
    // main loop, running every 25ms
    function mainLoop() {
        // check if the user is drawing
        // if (mouse.click && mouse.move && mouse.pos_prev) {
        //    // send line to to the server
        //    socket.emit('draw_line', { line: [ mouse.pos, mouse.pos_prev ] });
        //    mouse.move = false;
        // }
        // mouse.pos_prev = {x: mouse.pos.x, y: mouse.pos.y};
        setTimeout(mainLoop, 25);
    }
    // socket.emit('draw_line', {line: [{x: 0.0, y: 0.0}, {x: 0.5, y:0.5}]})
    // socket.emit('draw_line', {line: [{x: 0.5, y: 0.5}, {x: 1.0, y:0}]})
    mainLoop();
});
