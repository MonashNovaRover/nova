/**
 * Adapted from work by Roger Hardiman <opensource@rjh.org.uk>
 * https://github.com/agsh/onvif/blob/master/examples/example3.js
 */

var HOSTNAME = '10.0.1.100',
	PORT = 80,
	USERNAME = 'admin',
	PASSWORD = '***REMOVED***', // TODO: remove password somehow
	STOP_DELAY_MS = 100,
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

  var velocityX = 0;
  var velocityY = 0;
  var velocityZoom = 0;

  var stop_timer
  var timeoutX;
  var timeoutY;
  var timeoutZoom;

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

			if (key) {
				console.log('got "keypress"',key.name);
			} else {
				if (ch){console.log('got "keypress character"',ch);}
			}


			// On English keyboards '+' is "Shift and = key"
			// Accept the "=" key as zoom in
			if (key && key.name == 'up') {
        velocityY = SPEED;
			} else if (key && key.name == 'down') {
        velocityY = -SPEED;
			} else if (key && key.name == 'left') {
        velocityX = -SPEED;
			} else if (key && key.name == 'right') {
        velocityX = SPEED;
			} else if (ch  && ch       == '-') {
        velocityZoom = -SPEED;
			} else if (ch  && ch       == '+') {
        velocityZoom = SPEED;
			} else if (ch  && ch       == '=') {
        velocityZoom = SPEED;
			}
      // TODO: focus control?
			move(velocityX,velocityY,velocityZoom);
		});
	}


	function move(x_speed, y_speed, zoom_speed) {
		// Step 1 - Turn off the keyboard processing (so keypresses do not buffer up)
		// Step 2 - Clear any existing 'stop' timeouts. We will re-schedule a new 'stop' command in this function
		// Step 3 - Send the Pan/Tilt/Zoom 'move' command.
		// Step 4 - In the callback from the PTZ 'move' command we schedule the ONVIF Stop command to be executed after a short delay and re-enable the keyboard

		// Pause keyboard processing
		ignore_keypress = true;

		// Clear any pending 'stop' commands
		if (stop_timer) {clearTimeout(stop_timer);}


		// Move the camera
		cam_obj.continuousMove({x: x_speed,
			y: y_speed,
			zoom: zoom_speed } ,
		// completion callback function
		function(err, stream, xml) {
			if (err) {
				console.log(err);
			} else {
				console.log('move command sent ' + msg);
				// schedule a Stop command to run in the future
				stop_timer = setTimeout(stop,STOP_DELAY_MS);
			}
			// Resume keyboard processing
			ignore_keypress = false;
		});
	}


	function stop() {
		// send a stop command, stopping Pan/Tilt and stopping zoom
		console.log('sending stop command');
    move(0,0,0)
    velocityY = 0;
    velocityX=0;
    velocityZoom=0;
	}

);
