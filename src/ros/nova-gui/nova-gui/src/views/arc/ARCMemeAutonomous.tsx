import React from "react";
import { arcCameraSetup, ARCCompModes } from "../shared/CamerasPage/CameraPageConstants.tsx";
import { MemeCameraPage } from "../shared/CamerasPage/AutoMemeCamerasPage.tsx";

const ARCAutonomousView: React.FC = () => {
  return (
    <MemeCameraPage views={arcCameraSetup[ARCCompModes.ARC_OTHER]} />
  );
};

export default ARCAutonomousView;
