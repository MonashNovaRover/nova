import { Button, Card, CardBody, CardHeader, CardProps } from "@nextui-org/react";
import { useState, useEffect, useEffectEvent, useRef } from "react";
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

  // keep a ref to the latest joyState so the interval callback always reads current values
  const joyStateRef = useRef(joyState);
  useEffect(() => { joyStateRef.current = joyState; }, [joyState]);

  useEffect(() => {
    const publish = () => {
      const s = joyStateRef.current;
      const joyMsg: IRosSensorMsgsJoy = {
        header: {
          stamp: {sec: 0, nanosec: 0},
          frame_id: "gui-joy"
        },
        axes: [
          -s.left.distance * Math.cos(s.left.angle * Math.PI / 180),
          s.left.distance * Math.sin(s.left.angle * Math.PI / 180),
          -s.right.distance * Math.cos(s.right.angle * Math.PI / 180),
          s.right.distance * Math.sin(s.right.angle * Math.PI / 180),
          1,
          0
        ],
        buttons: [
          s.driveMode == DriveMode.PIVOT ? 1 : 0,
          s.driveMode == DriveMode.ACKERMANN ? 1 : 0,
          s.driveMode == DriveMode.STRAFE ? 1 : 0,
          s.driveMode == DriveMode.TANK ? 1 : 0,
          s.locked ? 1 : 0,
          0,
          !s.locked && s.locked !== undefined ? 1 : 0,
          0,
          0,
          s.controlMode == ControlMode.AUTONOMOUS ? 1 : 0,
          s.controlMode == ControlMode.MANUAL ? 1 : 0,
          s.speed == Speed.COARSEUP ? 1 : 0,
          s.speed == Speed.COARSEDOWN ? 1 : 0,
          s.speed == Speed.FINEDOWN ? 1 : 0,
          s.speed == Speed.FINEUP ? 1 : 0,
        ]
      };
      console.log(joyMsg)
      bifrostJoy.publishToTopic(joyMsg);
    };

    // publish immediately and then every 1s
    publish();
    const id = setInterval(publish, 1000);
    return () => clearInterval(id);
  }, [bifrostJoy])

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
  NONE,
  COARSEUP,
  FINEUP,
  FINEDOWN,
  COARSEDOWN
}

export default DriveJoyWidget;