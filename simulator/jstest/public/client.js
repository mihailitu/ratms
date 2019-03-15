  document.addEventListener("DOMContentLoaded", function() {
   var mouse = {
      click: false,
      move: false,
      pos: {x:0, y:0},
      pos_prev: false
   };
   // get canvas element and create context
   var canvas  = document.getElementById('drawing');
   var context = canvas.getContext('2d');
   var width   = window.innerWidth;
   var height  = window.innerHeight;
   var socket  = io.connect();

   // set canvas to full browser width/height
   canvas.width = width;
   canvas.height = height;

   console.log("client canvas[" + width + ", " + height + "]");

   // register mouse event handlers
   canvas.onmousedown = function(e){ mouse.click = true; };
   canvas.onmouseup = function(e){ mouse.click = false; };

   canvas.onmousemove = function(e) {
      // normalize mouse position to range 0.0 - 1.0
      mouse.pos.x = e.clientX / width;
      mouse.pos.y = e.clientY / height;
      mouse.move = true;
   };

   // draw line received from server
	socket.on('draw_map', function (data) {
    var map = data.map;

    for(var i = 0; i < map.length; i++) {
      var road = map[i];
      console.log('drawing: ' + road.id + ': ' + JSON.stringify(road));
      context.beginPath();
      context.lineWidth = road.lanes * 3;
      context.strokeStyle = "#ffff00";
      context.moveTo(road.start.x, road.start.y);
      context.lineTo(road.end.x, road.end.y);
      // TODO: draw normaziled coordinates
      // context.moveTo(road.start.x * width, road.start.y * height);
      // context.lineTo(road.end.x * width, road.end.y * height);
      context.stroke();
    }
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
