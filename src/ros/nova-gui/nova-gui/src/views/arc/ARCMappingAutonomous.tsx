import {React, useEffect, useRef} from "react";
import split_task from "../../livesplit.ts";
import {arcCameraSetup, ARCCompModes} from "../shared/CamerasPage/CameraPageConstants.tsx";
import {CameraPage} from "../shared/CamerasPage/AutoCamerasPage.tsx";

const ARCAutonomousView: React.FC = () => {
  useEffect(() => {
    split_task("test");
  }, []);
  return (
    <CameraPage views={arcCameraSetup[ARCCompModes.ARC_AUTONOMOUS]}/>
  );
};

export default ARCAutonomousView;
