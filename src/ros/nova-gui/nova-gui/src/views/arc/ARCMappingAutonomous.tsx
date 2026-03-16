import React from "react";
import {arcCameraSetup, ARCCompModes} from "../shared/CamerasPage/CameraPageConstants.tsx";
import {CameraPage} from "../shared/CamerasPage/AutoCamerasPage.tsx";

const ARCAutonomousView: React.FC = () => {
  return (
    <CameraPage views={arcCameraSetup[ARCCompModes.ARC_AUTONOMOUS]}/>
  );
};

export default ARCAutonomousView;
