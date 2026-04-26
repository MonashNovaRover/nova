import { Button, Card, CardBody, CardHeader, CardProps, Slider, Switch } from "@nextui-org/react";
import { useState, useEffect, useRef } from "react";
import { Lock } from "react-feather";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { IRosSensorMsgsJoy } from "../../../ros/rosTypes.ts";


export interface IArmJoyWidgetProps extends CardProps {}

/**
 * A component that sends joy commands for controlling the arm.
 * Must be used with rosbridge and teleop_drive_joy.
 * NOTE: WILL CONFLICT WITH ANY PLUGGED IN GAME CONTROLLERS
 */
const ArmJoyWidget: React.FC<IArmJoyWidgetProps> = (props) => {
  // bifrost stuff
  const bifrostJoyLeft = useBifrost({topic: RosTopic.ARM_JOY_LEFT})
  const bifrostJoyRight = useBifrost({topic: RosTopic.ARM_JOY_RIGHT})

  const [widgetLocked, setWidgetLock] = useState<boolean>(true);

  // Blur put over controls when widget is disabled
  const blurOverlay = (
    <div className="DriveModeWidgetOverlay flex flex-col justify-center content-center backdrop-blur-[1px] wrap-none z-10" />
  );
  const widgetLockMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Lock /> <span>Widget is disabled</span>
    </div>
  );

  // control state type
  type IJoyState = {
    j1: number
    j2: number
    j3: number
    j4: number
    j5: number
    j6: number
    x: number 
    y: number
    z: number
    roll: number
    pitch: number
    yaw: number
    fingers: number
    ee: number
    locked: boolean | undefined
    armMode: ArmMode
    speed: number
  }

  type jointKey = "j1" | "j2" | "j3" | "j4" | "j5" | "j6";
  type twistKey = "x" | "y" | "z" | "roll" | "pitch" | "yaw"
  const getKey = (mode: ArmMode, val:number) : jointKey | twistKey | "ee" | "fingers" | undefined => 
    val == 7 ? "ee" : val == 8 ? "fingers"
    : mode == ArmMode.JointSpace ? (val <= 6 && val >=1 ? `j${val}` as jointKey : undefined)
    : mode == ArmMode.TaskSpace ? (val == 1 ? "x" : val == 2 ? "y" : val == 3 ? "z" : val == 4 ? "roll" : val == 5 ? "pitch" : val == 6 ? "yaw" : undefined) as twistKey | undefined
    : undefined
  const [joyState, setJoyState] = useState<IJoyState>({
    j1: 0,
    j2: 0,
    j3: 0,
    j4: 0,
    j5: 0,
    j6: 0,
    x: 0,
    y: 0,
    z: 0,
    roll: 0,
    pitch: 0,
    yaw: 0,
    fingers: 0,
    ee: 0,
    locked: undefined,
    armMode: ArmMode.JointSpace,
    speed: 0,
  });

  const [fakeSpeed, setFakeSpeed] = useState<number>(0);

  // update state whenever slider values change
  const updateSliderState = (slider: number, val: number) => {
    const key = getKey(joyState.armMode, slider)!
    setJoyState(prev => ({...prev,
      [key]: val,
    }))
  }; 

  // update state when buttons pressed
  const setArmMode = (newArmMode: ArmMode ) => {
    setJoyState(prev => ({...prev,
      armMode: newArmMode
    }))
  };

  const updateSpeedState = (speed: number) => {
    const actualSpeed = speed <= 10 ? speed / 10 : speed - 10;
    setJoyState(prev => ({
      ...prev,
      speed: actualSpeed
    }))
    setFakeSpeed(speed);
  };

  const setLock = (locked: boolean | undefined) => {
    setJoyState(prev=>({...prev,
      locked: locked
    }))
  };


  // keep a ref to the latest joyState so the interval callback always reads current values
  const joyStateRef = useRef(joyState);
  useEffect(() => { joyStateRef.current = joyState; }, [joyState]);

  // values pulled from src/ros/rover/arm/teleop_arm/params/joysticks.config.yaml accurate as of 26/04/2026 3c86a3b0fe14d37aa6a48a8e804de0ba19a40544
  /* Teleop Arm Msg (Currently implemented):
    axes:
      /arm/joy/left: [left_0, left_1, left_2, speed, none, end effector]
      /arm/joy/right: [right_0, right_1, right_2, none, none, fingers]
    buttons: 
      /arm/joy/left: [task_mode, none, none, none, none, lock, none, none, unlock] + [none]x7
      /arm/joy/right: [none]x16
    
    where left_0, right_0 etc are defined by the current mode:
      Joint Space Mode (FK):
        /arm/joy/left: [j3 (inverted), j2, j1]
        /arm/joy/right: [j6 (inverted), j5, j4]
      Task Space Mode (IK):
        /arm/joy/left: [y, x, z]
        /arm/joy/right: [roll, pitch, yaw]
  */
  /*
    Teleop Arm Joy Msg (not implemented):
    axes:
      /joy_L: [left_0, left_1, left_2, offset]
      /joy_R: [right_0, right_1, right_2, speed]
    buttons: 
      /joy_L: [none, none, none, none, limit_on, lock, none, none, unlock, limit_off]
      /joy_R: [task_mode]
    
    where left_0, right_0 etc are defined by the current mode:
      Joint Space Mode (FK):
        /joy_L: [j2, j3 (inverted), j1]
        /joy_R: [j5, j4, j6 (inverted)]
      Task Space Mode (IK):
        /joy_L: [twist_roll, twist_pitch (inverted), twist_yaw]
        /joy_R: [twist_y, twist_x, twist_z (inverted)]
  */
    // publish on joy on interval for heartbeat
    useEffect(() => {
    const publish = () => {
      const s = joyStateRef.current;
      const joyLeftMsg: IRosSensorMsgsJoy = {
        header: {
          stamp: {sec: 0, nanosec: 0},
          frame_id: "gui-joy"
        },
        axes: s.armMode == ArmMode.JointSpace ? [
            -s.j3, s.j2, s.j1, s.speed, 0, s.ee
          ] : [
            s.y, s.x, s.z, s.speed, 0, s.ee
        ],
        buttons: [
          s.armMode == ArmMode.TaskSpace ? 1 : 0, 
          0, 0, 0, 0, 
          s.locked ? 1 : 0,
          0, 0, 
          !s.locked && s.locked !== undefined ? 1 : 0,
          0, 0, 0, 0, 0, 0, 0]
      };
      const joyRightMsg: IRosSensorMsgsJoy = {
        header: {
          stamp: {sec: 0, nanosec: 0},
          frame_id: "gui-joy"
        },
        axes: s.armMode == ArmMode.JointSpace ? [
            -s.j6, s.j5, s.j4, 0, 0, s.fingers
          ] : [
            s.roll, s.pitch, s.yaw, 0, 0, s.fingers
        ],
        buttons: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      };
      if (!widgetLocked) {
        bifrostJoyLeft.publishToTopic(joyLeftMsg);
        bifrostJoyRight.publishToTopic(joyRightMsg);
      }
    };

    // publish immediately and then every 1s
    publish();
    const id = setInterval(publish, 1000);
    return () => clearInterval(id);
  }, [bifrostJoyLeft, bifrostJoyRight, widgetLocked])

  const jointSlider = (sliderNumber: number) =>
    (<div>
      <span className="block relative w-full text-center align-center">
        {getKey(joyState.armMode, sliderNumber)!}
        <span className="absolute left-[50%] top-[100%] -translate-x-1/2 text-center z-[2] pointer-events-none">+</span>
        <span className="absolute left-[50%] top-[730%] -translate-x-1/2 text-center text-xs z-[3] text-black pointer-events-none">0</span>
        <span className="absolute left-[50%] top-[1315%] -translate-x-1/2 text-center z-[2] pointer-events-none">-</span>
      </span>
      <Slider
        size="lg"
        step={0.2}
        maxValue={1}
        minValue={-1}
        orientation="vertical"
        aria-label={getKey(joyState.armMode, sliderNumber)!}
        defaultValue={0}
        value={joyState[getKey(joyState.armMode, sliderNumber)!]}
        classNames={{base: "h-[20rem]", thumb: "after:bg-white w-10 ", step: "data-[in-range=true]:bg-white/50"}}
        showSteps={true}
        fillOffset={0}
        onChange={(v)=>updateSliderState(sliderNumber, +v)}
        onChangeEnd={()=>updateSliderState(sliderNumber, 0)}
      />
    </div>)
  
  const SpeedSlider = (<div>
    <span className="block relative w-full text-center align-center">
      Speed
      <span className="absolute left-[50%] top-[100%] -translate-x-1/2 text-center z-[2] pointer-events-none">10</span>
      <span className="absolute left-[50%] top-[730%] -translate-x-1/2 text-center text-xs z-[2] pointer-events-none">1</span>
      <span className="absolute left-[50%] top-[1315%] -translate-x-1/2 text-center z-[2] pointer-events-none">0</span>
    </span>
    <Slider
      size="lg"
      step={1}
      maxValue={20}
      minValue={0}
      orientation="vertical"
      aria-label="Speed"
      defaultValue={0}
      value={fakeSpeed}
      classNames={{base: "h-[20rem]", thumb: "after:bg-white w-10 relative", step: "data-[in-range=true]:bg-white/50"}}
      renderThumb={(props)=><div {...props}><span className="absolute text-black pointer-events-none z-[2]">{joyState.speed}</span></div>}
      showSteps={true}
      onChange={(v)=>{updateSpeedState(+v)}}
    />
  </div>)

  // return a card with all components
  return (
    <Card className="no-scroll" {...props}>
      <CardHeader className="gap-3">
        <span>Arm Control</span>
        <Switch
          className=""
          size="sm"
          isSelected={!widgetLocked}
          onChange={() =>{setWidgetLock(!widgetLocked)}}
        />
        <div className="grow" />
        {widgetLocked ? widgetLockMessage : <></>}
      </CardHeader>
      <CardBody className="flex">
        <div className="flex gap-4 justify-center">
          <div className="grid grid-cols-2 content-center gap-2">
            <Button className="min-w-0"
              onPressStart={()=>{setLock(true)}}
              onPressEnd={()=>{setLock(undefined)}}>
              Lock
            </Button>
            <Button className="min-w-0"
              onPressStart={()=>{setLock(false)}}
              onPressEnd={()=>{setLock(undefined)}}>
              Unlock
            </Button>
            <Button className="min-w-0"
              onPressStart={()=>{setArmMode(ArmMode.JointSpace)}}>
              Joint Space
            </Button>
            <Button className="min-w-0"
              onPressStart={()=>{setArmMode(ArmMode.TaskSpace)}}>
              Task Space
            </Button>
          </div>
          <div className="flex gap-4">
            {[1,2,3,4,5,6,7,8].map(v=>jointSlider(v))}
            {SpeedSlider}
          </div>
        </div>
        {widgetLocked ? blurOverlay : <></>}
      </CardBody>
    </Card>
  );
};

enum ArmMode {
  JointSpace,
  TaskSpace,
  EndEffector,
};


export default ArmJoyWidget;