#!/usr/bin/env node
/**
 * Adapted from work by Roger Hardiman <opensource@rjh.org.uk>
 * https://github.com/agsh/onvif/blob/master/examples/example3.js
 */

const fs = require('fs');
const os = require('os');
const path = require('path');

var HOSTNAME = '10.0.1.100',
	PORT = 80,
	USERNAME = 'admin',
  PASSWORD = fs.readFileSync(
    path.join(os.homedir(), 'nova', 'secrets', 'reolink-password.txt'), 'utf8'
  ).trim(),
	STOP_DELAY_MS = 200

var Cam = require('onvif').Cam;
var keypress = require('keypress');

var autoMode = false;
var auto_timer;
var auto_count;


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
    x: 0,
    y: 0,
    zoom: 0
  }

  var left = { ...velocity, x: -1};
  var right = { ...velocity, x: 1};
  var up = { ...velocity, y: 1};
  var down = { ...velocity, y: -1};

  // TODO DRY
  autoSequence = [
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: left, time: 600},
    {vel: up, time: 800},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: right, time: 600},
    {vel: down, time: 1000}
  ];

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
    console.log('Use a to toggle auto mode (repeated panning left and right forever)');

		// keypress handler
		process.stdin.on('keypress', function(ch, key) {

			/* Exit on 'q' or 'Q' or 'CTRL C' */
			if ((key && key.ctrl && key.name == 'c')
				|| (key && key.name == 'q')) {
        autoMode = false;
          schedule_auto_timer(); // will cancel it
        stop();
				process.exit();
			}

			if (ignore_keypress) {
				return;
			}

      var new_velocity = {
        x: 0,
        y: 0,
        zoom: 0
      }

			// On English keyboards '+' is "Shift and = key"
			// Accept the "=" key as zoom in
      // TODO: flip controls if upside down? Easiest way would be to negate speed
      // although that won't work for zoom.
			if (key && key.name == 'up') {
        new_velocity = up;
			} else if (key && key.name == 'down') {
        new_velocity = down;
			} else if (key && key.name == 'left') {
        new_velocity = left;
			} else if (key && key.name == 'right') {
        new_velocity = right;
			} else if (ch  && ch       == '-') {
        new_velocity.zoom = -1;
			} else if (ch  && ch       == '+') {
        new_velocity.zoom = 1;
			} else if (ch  && ch       == '=') {
        new_velocity.zoom = 1;
			} else if (ch && ch        == 'a') {
        autoMode = !autoMode;
        console.log("Auto Mode", autoMode);
        if (autoMode) {
          auto_count = 0;
          auto_cb();
        } else {
          schedule_auto_timer(); // will cancel it
          stop();
        }
      }
      if (!autoMode) {
        move(new_velocity);
      }
		});
	}

  function schedule_auto_timer(time) {
         if (auto_timer) {
            clearTimeout(auto_timer);
         }
		  if (autoMode) {
         if (auto_timer) {
            clearTimeout(auto_timer);
         }
		  auto_timer = setTimeout(auto_cb,time);
    }
  }

  function auto_cb() {

		cam_obj.continuousMove(autoSequence[auto_count].vel ,
		// completion callback function
		function(err, stream, xml) {
			if (err) {
				console.log(err);
			} else {
        schedule_auto_timer(autoSequence[auto_count].time);
        auto_count = (auto_count + 1) % autoSequence.length;
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


	function move(new_velocity) {
    // check if we need to update the currently running move command
    if (
      (new_velocity.X == velocity.X) 
      && (new_velocity.Y == velocity.Y) 
      // for some reason zoom works better if we send the command repeatedly.
      && !new_velocity.Zoom //(new_velocity.Zoom == velocity.Zoom) 
    ) {
      // reschedule timeout
      schedule_stop();
      return;
    }

    velocity = {...new_velocity};

		// Pause keyboard processing
		ignore_keypress = true;

		// Clear any pending 'stop' commands
    clear_stop()

		// Move the camera
		cam_obj.continuousMove(velocity,
		// completion callback function
		function(err, stream, xml) {
			if (err) {
				console.log(err);
			} else {
				console.log('move command sent ', velocity);
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