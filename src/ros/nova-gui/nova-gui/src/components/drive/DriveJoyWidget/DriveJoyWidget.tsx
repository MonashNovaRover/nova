import { Button, Card, CardBody, CardHeader, CardProps } from "@nextui-org/react";
import { useState, useEffect, useEffectEvent } from "react";
import { Info, Lock } from "react-feather";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RootState } from "../../../redux/RootState";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { DriveMode} from "../DriveModeWidget/DriveModeDisplayData";
import { IRosSensorMsgsJoy } from "../../../ros/rosTypes.ts";

import Joystick, { IJoystickChangeValue } from 'rc-joystick';

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
  const bifrostJoy = useBifrost({topic: RosTopic.DRIVE_JOY})
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
    locked: boolean | undefined
    driveMode: DriveMode | undefined
    controlMode: ControlMode | undefined
    speed: Speed
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
    speed: 0
  });

  // update state whenever joystick values change
  const updateJoystickState = (side: JoystickSide, val:IJoystickChangeValue) => {
    const { distance, angle } = val;
    const angleN = angle ?? 0
    if (side == JoystickSide.LEFT && (distance !== joyState.left.distance || angleN !== joyState.left.angle)) {
      setJoyState(prev => ({...prev,
        left: {
          distance: distance,
          angle: angleN
        }
      }));
    } else if (side == JoystickSide.RIGHT && (distance !== joyState.right.distance || angleN !== joyState.right.angle)) {
      setJoyState(prev => ({...prev,
        right: {
          distance: distance,
          angle: angleN
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

     Teleop Drive Joy Msg
      axes: [left horizontal, left vertical, right horizontal, right vertical, stop autolock, speed down linear]
      buttons: [pivot, ackermann, strafe, tank, lock, none, unlock, none, none, auto mode, manual mode, speed up big, speed down big, speed down small, speed up small]
  */
  useEffect(()=>{
    const joyMsg: IRosSensorMsgsJoy = {
      header: {
        stamp: {sec: 0, nanosec: 0}, //help-----------------------------
        frame_id: "gui-joy"
      },
      axes: [-joyState.left.distance * Math.cos(joyState.left.angle * Math.PI / 180), 
             joyState.left.distance * Math.sin(joyState.left.angle * Math.PI / 180),
             -joyState.right.distance * Math.cos(joyState.right.angle * Math.PI / 180), 
             joyState.right.distance * Math.sin(joyState.right.angle * Math.PI / 180),
             1, // always stop autolock
             0, // speed down linear (handbrake) unused in gui
            ],

      buttons: [
        joyState.driveMode == DriveMode.PIVOT ? 1 : 0,
        joyState.driveMode == DriveMode.ACKERMANN ? 1 : 0,
        joyState.driveMode == DriveMode.STRAFE ? 1 : 0,
        joyState.driveMode == DriveMode.TANK ? 1 : 0,
        joyState.locked ? 1 : 0, // lock
        0, 
        !joyState.locked && joyState.locked !== undefined ? 1 : 0, // unlock
        0,
        0,
        joyState.controlMode == ControlMode.AUTONOMOUS ? 1 : 0,
        joyState.controlMode == ControlMode.MANUAL ? 1 : 0,
        joyState.speed == Speed.COARSEUP ? 1 : 0,
        joyState.speed == Speed.COARSEDOWN ? 1 : 0,
        joyState.speed == Speed.FINEDOWN ? 1 : 0,
        joyState.speed == Speed.FINEUP ? 1 : 0,
      ]
    }
    bifrostJoy.publishToTopic(joyMsg)
    console.log(joyState)
  }, [joyState, bifrostJoy])

  // Joystick component properties
  const Stick = (side: JoystickSide) => <div className="relative m-8">
    <Joystick onChange={(val)=>updateJoystickState(side, val)} />
  </div>


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
          <div className="grid grid-cols-4 gap-3">
            <Button className="col-start-1 min-w-0"
              onPressStart={()=>setDriveMode(DriveMode.PIVOT)}
              onPressEnd={()=>setDriveMode(undefined)}>
              Pivot
            </Button>
            <Button className="col-start-2 min-w-0"
              onPressStart={()=>setDriveMode(DriveMode.ACKERMANN)}
              onPressEnd={()=>setDriveMode(undefined)}>
              Ackermann
            </Button>
            <Button className="col-start-3 min-w-0"
              onPressStart={()=>setDriveMode(DriveMode.STRAFE)}
              onPressEnd={()=>setDriveMode(undefined)}>
              Strafe
            </Button>
            <Button className="col-start-4 min-w-0"
              onPressStart={()=>setDriveMode(DriveMode.TANK)}
              onPressEnd={()=>setDriveMode(undefined)}>
              Tank
            </Button>
          </div>
          <div className="space-y-2">
            <div className="flex row-start-1 justify-center">
              <span className="text-sm">Speed</span>
            </div>
            <div className="grid grid-cols-4 gap-3">
              <Button className="col-start-1 min-w-0"
                onPressStart={()=>updateSpeedState(Speed.COARSEDOWN)}
                onPressEnd={()=>updateSpeedState(Speed.NONE)}>
                --
              </Button>
              <Button className="col-start-2 min-w-0"
                onPressStart={()=>updateSpeedState(Speed.FINEDOWN)}
                onPressEnd={()=>updateSpeedState(Speed.NONE)}>
                -
              </Button>
              <Button className="col-start-3 min-w-0"
                onPressStart={()=>updateSpeedState(Speed.FINEUP)}
                onPressEnd={()=>updateSpeedState(Speed.NONE)}>
                +
              </Button>
              <Button className="col-start-4 min-w-0"
                onPressStart={()=>updateSpeedState(Speed.COARSEUP)}
                onPressEnd={()=>updateSpeedState(Speed.NONE)}>
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

enum Speed {
  COARSEUP,
  FINEUP,
  NONE,
  FINEDOWN,
  COARSEDOWN
}

export default DriveJoyWidget;