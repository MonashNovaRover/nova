import React from "react";
import {arcCameraSetup, ARCCompModes} from "../shared/CamerasPage/CameraPageConstants.tsx";
import {AutoCameraPage} from "../shared/CamerasPage/AutoCamerasPage.tsx";

const ARCAutonomousView: React.FC = () => {
  return (
    <AutoCameraPage views={arcCameraSetup[ARCCompModes.ARC_AUTONOMOUS]}/>
  );
};

export default ARCAutonomousView;
