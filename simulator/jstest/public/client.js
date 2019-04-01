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

    socket.on('draw_state', function (data) {
        var canvas  = document.getElementById('drawing');
        var context = canvas.getContext('2d');

        // clear canvas
        context.clearRect(0, 0, viewWidth, viewHeight);

        var frameTxt = "Frame: " + data.time;
        var textX = viewWidth - context.measureText("Frame: 000000.000000").width;
        var textY = 15;
        var textH = 15;
        context.fillText(frameTxt, textX, textY);
        textY += textH;
        context.fillText("Roads: " + road_map.length, textX, textY);
        textY += textH;
        context.fillText("Vehicles: " + data.data.length, textX, textY);

        var delay = 500; //ms
        var statFrameTime = new Date().getTime();

        // first, draw the roads
        for(var i = 0; i < road_map.length; i++) {
            var road = road_map[i];
            // console.log('drawing: ' + road.id + ': ' + JSON.stringify(road));
            context.beginPath();
            context.lineWidth = road.lanes * 3;
            context.moveTo(road.start.x * viewWidth, road.start.y * viewHeight);
            context.lineTo(road.end.x * viewWidth, road.end.y * viewHeight);
            context.strokeStyle = "#eff2f7";
            // context.strokeStyle = 'green';
            context.stroke();
        }

        for(var i = 0; i < data.data.length; ++i) {
            for(var v = 0; v < data.data[i].vehicles.length; ++v) {
                context.beginPath();
                context.arc(data.data[i].vehicles[v].x * viewWidth, data.data[i].vehicles[v].y * viewHeight, 3, 0, 2 * Math.PI);
                context.fillStyle = "blue";
                context.strokeStyle = "red";
                context.lineWidth = 1;
                context.stroke();
                context.fill();
            }
        }

        sleep(statFrameTime, delay);

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
