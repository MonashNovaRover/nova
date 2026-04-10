import { useState } from "react";
import {arcCameraSetup, ARCCompModes} from "../../shared/CamerasPage/CameraPageConstants.tsx";
import {CameraSidebar} from "../../../components/cameras/CameraPage/CameraSidebar.tsx";
import {CamerasPage} from "../../../components/cameras/CameraPage/CamerasPage.tsx";

export const TestCameraPage = () => {

  const [showSidebar, setShowSidebar] = useState(true)

  return (
    <div
      className="flex flex-row w-full items-stretch"
      style={{ height: "calc(100vh - 4.05rem)" }}
    >
      <CameraSidebar showSidebar={showSidebar} setShowSidebar={setShowSidebar}/>
      <div className="grow">
        <CamerasPage
          views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]}
          toggleSidebar={() => setShowSidebar(!showSidebar)}
          gridSize={2}
        />
      </div>
    </div>
  )
}
