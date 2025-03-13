/**
 * Adapted from work by Roger Hardiman <opensource@rjh.org.uk>
 * https://github.com/agsh/onvif/blob/master/examples/example3.js
 */

var HOSTNAME = '10.0.1.100',
	PORT = 80,
	USERNAME = 'admin',
	PASSWORD = 'Lab188b37', // TODO: remove password somehow
	STOP_DELAY_MS = 200,
  SPEED = 2;

var Cam = require('onvif').Cam;
var keypress = require('keypress');


new Cam({
	hostname: HOSTNAME,
	username: USERNAME,
	password: PASSWORD,
	port: PORT,
	timeout: 10000
}, function CamFunc(err) {
	if (err) {
		console.log(err);
		return;
	}

	var cam_obj = this;
	var ignore_keypress = false;

  var velocity = {
    X: 0,
    Y: 0,
    Zoom: 0
  }

  var stop_timer;

	cam_obj.getStreamUri({
		protocol: 'RTSP'
	},	// Completion callback function
	// This callback is executed once we have a StreamUri
	function(err, stream, xml) {
		if (err) {
			console.log(err);
			return;
		} else {
			console.log('------------------------------');
			console.log('Host: ' + HOSTNAME + ' Port: ' + PORT);
			console.log('Stream: = ' + stream.uri);
			console.log('------------------------------');

			// start processing the keyboard
			read_and_process_keyboard();
		}
	}
	);

	function read_and_process_keyboard() {
		// listen for the "keypress" events
		keypress(process.stdin);
		process.stdin.setRawMode(true);
		process.stdin.resume();

		console.log('');
		console.log('Use Arrow Keys to move camera. + and - to zoom. q to quit');

		// keypress handler
		process.stdin.on('keypress', function(ch, key) {

			/* Exit on 'q' or 'Q' or 'CTRL C' */
			if ((key && key.ctrl && key.name == 'c')
				|| (key && key.name == 'q')) {
				process.exit();
			}

			if (ignore_keypress) {
				return;
			}

      var new_velocity = {
        X: 0,
        Y: 0,
        Zoom: 0
      }

			// On English keyboards '+' is "Shift and = key"
			// Accept the "=" key as zoom in
			if (key && key.name == 'up') {
        new_velocity.Y = SPEED;
			} else if (key && key.name == 'down') {
        new_velocity.Y = -SPEED;
			} else if (key && key.name == 'left') {
        new_velocity.X = -SPEED;
			} else if (key && key.name == 'right') {
        new_velocity.X = SPEED;
			} else if (ch  && ch       == '-') {
        new_velocity.Zoom = -SPEED;
			} else if (ch  && ch       == '+') {
        new_velocity.Zoom = SPEED;
			} else if (ch  && ch       == '=') {
        new_velocity.Zoom = SPEED;
			}
      // TODO: focus control?
      if (
        (new_velocity.X == velocity.X) 
        && (new_velocity.Y == velocity.Y) 
        && !new_velocity.Zoom //(new_velocity.Zoom == velocity.Zoom) 
      ) {
        // reschedule timeout
        schedule_stop();
      } else {
        velocity = new_velocity;
			  move(velocity.X,velocity.Y,velocity.Zoom);
      }
		});
	}

  function clear_stop() {
		if (stop_timer) {clearTimeout(stop_timer);}
  }

  function schedule_stop() {
    clear_stop()
		stop_timer = setTimeout(stop,STOP_DELAY_MS);
  }


	function move(x_speed, y_speed, zoom_speed) {
		// Pause keyboard processing
		ignore_keypress = true;

		// Clear any pending 'stop' commands
    clear_stop()

		// Move the camera
		cam_obj.continuousMove({x: x_speed,
			y: y_speed,
			zoom: zoom_speed } ,
		// completion callback function
		function(err, stream, xml) {
			if (err) {
				console.log(err);
			} else {
				console.log('move command sent ', x_speed, y_speed, zoom_speed);
        schedule_stop()
			}
			// Resume keyboard processing
			ignore_keypress = false;
		});
	}

	function stop() {
		// send a stop command, stopping Pan/Tilt and stopping zoom
		//console.log('sending stop command');
    cam_obj.stop({panTilt: true, zoom: true},
			function(err,stream, xml){
				if (err) {
					console.log(err);
				} else {
					//console.log('stop command sent');
				}
			});
    velocity.Y = 0;
    velocity.X=0;
    velocity.Zoom=0;
	}

});
