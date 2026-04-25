import { Button, Card, CardBody, CardHeader, CardProps } from "@nextui-org/react";
import { useState, useEffect } from "react";
import { Info, Lock } from "react-feather";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RootState } from "../../../redux/RootState";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { DriveMode} from "../DriveModeWidget/DriveModeDisplayData";
import { IRosSensorMsgsJoy } from "../../../ros/rosTypes.ts";

import ReactNipple from 'react-nipple';

// Properties for the DriveModeWidget component.
export interface IDriveJoyWidgetProps extends CardProps {}

/**
 * A component that sends joy commands for driving the rover.
 * It emulates the joy format of an xbox controller.
 * Must be used with rosbridge and teleop_drive_joy.
 * Combines the two joysticks of the xbox into one for mobile use.
 */
const DriveJoyWidget: React.FC<IDriveJoyWidgetProps> = (props) => {
  // bifrost stuff
  const bifrostDriveInfo = useBifrost({ topic: RosTopic.DRIVE_INFO });
  const bifrostJoy = useBifrost({topic: RosTopic.JOY})
  const isLocked = useSelector((state: RootState) => state.driveStore.locked);
  const isConnected = useSelector(
    (state: RootState) => state.driveStore.connected
  );

  useEffect(() => {
    bifrostDriveInfo.syncWithTopic();
  }, [bifrostDriveInfo]);

  // Message to show when disconnected
  const disconnectedMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Info /> <span>Disconnected</span>
    </div>
  );

  // Message to show when locked
  const lockedMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Lock /> <span>Controller is locked</span>
    </div>
  );

  // control state type
  type IJoyState = {
    left: {
      distance: number,
      angle: number,
    },
    right: {
      distance: number,
      angle: number,
    },
    locked: Boolean | undefined
    driveMode: DriveMode | undefined
    controlMode: ControlMode | undefined
    speed: number
  }
  const [joyState, setJoyState] = useState<IJoyState>({
    left: {
      distance: 0, // distance from center of stick [0, 1]
      angle: 0,    // angle in deg from 3oclock
    },
    right: {
      distance: 0,
      angle: 0,
    },
    locked: undefined,
    driveMode: undefined,
    controlMode: undefined,
    speed: 0    // [-2, 2]
  });

  // update state whenever joystick values change
  const updateJoystickState = (distance:number, angle:number, side: JoystickSide) => {
    if (side == JoystickSide.LEFT && (distance !== joyState.left.distance || angle !== joyState.left.angle)) {
      setJoyState(prev => ({...prev,
        left: {
          distance: distance,
          angle: angle
        }
      }));
    } else if (side == JoystickSide.RIGHT && (distance !== joyState.right.distance || angle !== joyState.right.angle)) {
      setJoyState(prev => ({...prev,
        right: {
          distance: distance,
          angle: angle
        }
      }));
    }
  }; 

  // update state when buttons pressed
  const setDriveMode = (newDriveMode: DriveMode | undefined) => {
    setJoyState(prev => ({...prev,
      driveMode: newDriveMode
    }))
  };

  const updateSpeedState = (speed: number) => setJoyState(prev => ({
    ...prev,
    speed: speed
  }));

  const setLock = (locked: boolean | undefined) => {
    setJoyState(prev=>({...prev,
      locked: locked
    }))
  };

  const setControlMode = (controlMode: ControlMode | undefined) => {
    setJoyState(prev=>({...prev,
      controlMode: controlMode
    }))
  };

  /* send joy messages when state changes
     must convert state to joy message axes and buttons
     joy msg format on xbox:
     axes: number[-1to1] = [left_x, left_y, left_trigger, right_x, right_y, right_trigger, dpad_x, dpad_y]
     buttons: number[0:1] = [a, b, x, y, left_bumper, right_bumper, screenshot, menu, xbox, left_stick_press, right_stick_press, upload]
     x axes are inverted e.g right = -1
  */
  useEffect(()=>{
    const joyMsg: IRosSensorMsgsJoy = {
      header: {
        stamp: {sec: 0, nanosec: 0}, //help-----------------------------
        frame_id: "gui-joy"
      },
      axes: [-joyState.left.distance * Math.cos(joyState.left.angle * Math.PI / 180), 
             joyState.left.distance * Math.sin(joyState.left.angle * Math.PI / 180),
             1.0, // left trigger
             -joyState.right.distance * Math.cos(joyState.right.angle * Math.PI / 180), 
             joyState.right.distance * Math.sin(joyState.right.angle * Math.PI / 180),
             1.0, // right trigger
             Math.abs(joyState.speed) == 1 ? joyState.speed : 0, // dpad x
             Math.abs(joyState.speed) == 2 ? joyState.speed / 2 : 0, // dpad y
            ],

      buttons: [joyState.controlMode == ControlMode.AUTONOMOUS ? 1 : 0, // auto mode
                joyState.controlMode == ControlMode.MANUAL ? 1 : 0, // manual mode
                0, // x button
                joyState.driveMode == DriveMode.TANK ? 1 : 0,   // tank mode
                joyState.driveMode == DriveMode.STRAFE ? 1 : 0, // strafe mode
                joyState.driveMode == DriveMode.PIVOT ? 1 : 0,  // pivot mode
                joyState.locked ? 1 : 0, // lock game pad
                !joyState.locked && joyState.locked !== undefined ? 1 : 0, // unlock game pad
                0, // xbox button
                0, // left stick button 
                0, // right stick button
                0], // upload button
    }
    bifrostJoy.publishToTopic(joyMsg)
  }, [joyState])

  // Joystick component properties
  const joystickRadius = 150
  const Stick = (side: JoystickSide) => <div className={`relative m-8`} style={{width: `${joystickRadius}px`}}>
    <ReactNipple
      className={(side == JoystickSide.LEFT ? "joystick-left" : "joystick-right")}
      options={{
          mode: 'static', 
          position: { top: '50%', left: '50%' }, 
          maxNumberOfNipples: 2,
          size: joystickRadius
      }}
      style={{
          width: '100%',
          height: '100%',
          position: 'absolute',
      }}
      // https://www.npmjs.com/package/nipplejs
      onMove={(_:any, data:any) => {
        let distance = data.distance / (joystickRadius / 2)
        let angle = data.angle.degree
        updateJoystickState(distance, angle, side)
      }}
      onEnd={() => updateJoystickState(0, 0, side)}
    />
  </div>
  
  useEffect(() => {
    const preventPinchZoom = (e: TouchEvent) => {
      if (e.touches.length > 1) {
        e.preventDefault();
      }
    };
  
    document.addEventListener('touchmove', preventPinchZoom, { passive: false });
  
    return () => {
      document.removeEventListener('touchmove', preventPinchZoom);
    };
  }, []);

  // return a card with all components
  // its a big blob of code i'm sorry if anyone decides to read this
  return (
    <Card className="no-scroll" {...props}>
      <CardHeader>
        <span>Drive Control</span>
        <div className="grow" />
        {!isConnected ? disconnectedMessage : isLocked ? lockedMessage : <></>}
      </CardHeader>
      <CardBody className="flex-row mx-auto justify-between p-3 gap-4">
        {Stick(JoystickSide.LEFT)}
        <div className="space-y-2 w-full">
          <div className="grid grid-cols-4 gap-3">
            <Button className="col-start-1 min-w-0"
              onPressStart={()=>{setLock(true)}}
              onPressEnd={()=>{setLock(undefined)}}>
              Lock
            </Button>
            <Button className="col-start-2 min-w-0"
              onPressStart={()=>{setControlMode(ControlMode.AUTONOMOUS)}}
              onPressEnd={()=>{setControlMode(undefined)}}>
              Auto
            </Button>
            <Button className="col-start-3 min-w-0"
              onPressStart={()=>{setControlMode(ControlMode.MANUAL)}}
              onPressEnd={()=>setControlMode(undefined)}>
              Manual
            </Button>
            <Button className="col-start-4 min-w-0"
              onPressStart={()=>{setLock(false)}}
              onPressEnd={()=>{setLock(undefined)}}>
              Unlock
            </Button>
          </div>
          <div className="grid grid-cols-3 gap-3">
            <Button className="col-start-1 min-w-0"
              onPressStart={()=>setDriveMode(DriveMode.TANK)}
              onPressEnd={()=>setDriveMode(undefined)}>
              Tank
            </Button>
            <Button className="col-start-2 min-w-0"
              onPressStart={()=>setDriveMode(DriveMode.STRAFE)}
              onPressEnd={()=>setDriveMode(undefined)}>
              Strafe
            </Button>
            <Button className="col-start-3 min-w-0"
              onPressStart={()=>setDriveMode(DriveMode.PIVOT)}
              onPressEnd={()=>setDriveMode(undefined)}>
              Pivot
            </Button>
          </div>
          <div className="space-y-2">
            <div className="flex row-start-1 justify-center">
              <span className="text-sm">Speed</span>
            </div>
            <div className="grid grid-cols-4 gap-3">
              <Button className="col-start-1 min-w-0"
                onPressStart={()=>updateSpeedState(-2)}
                onPressEnd={()=>updateSpeedState(0)}>
                --
              </Button>
              <Button className="col-start-2 min-w-0"
                onPressStart={()=>updateSpeedState(-1)}
                onPressEnd={()=>updateSpeedState(0)}>
                -
              </Button>
              <Button className="col-start-3 min-w-0"
                onPressStart={()=>updateSpeedState(1)}
                onPressEnd={()=>updateSpeedState(0)}>
                +
              </Button>
              <Button className="col-start-4 min-w-0"
                onPressStart={()=>updateSpeedState(2)}
                onPressEnd={()=>updateSpeedState(0)}>
                ++
              </Button>
            </div>
          </div>
        </div>
        {Stick(JoystickSide.RIGHT)}
      </CardBody>
    </Card>
  );
};

enum ControlMode {
  MANUAL,
  AUTONOMOUS,
};

enum JoystickSide {
  LEFT,
  RIGHT,
}

export default DriveJoyWidget;