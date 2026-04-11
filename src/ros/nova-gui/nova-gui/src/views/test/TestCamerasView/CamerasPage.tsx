import { useState } from "react";
import {arcCameraSetup, ARCCompModes} from "../../shared/CamerasPage/CameraViewConstants.tsx";
import {CameraSidebar} from "../../../components/cameras/CameraPage/CameraSidebar.tsx";
import {CamerasPage} from "../../../components/cameras/CameraPage/CamerasPage.tsx";
import {useCameraStreamer} from "../../../components/cameras/hooks/useCameraStreamer.ts";

export const TestCameraPage = () => {
  const { refreshAvailabilities } = useCameraStreamer();
  const [showSidebar, setShowSidebar] = useState(false)
  const [gridSize, setGridSize] = useState(4)

  return (
    <div
      className="flex flex-row w-full items-stretch"
      style={{ height: "calc(100vh - 4.05rem)" }}
    >
      <CameraSidebar
        refreshAvailabilities={refreshAvailabilities}
        showSidebar={showSidebar}
        setShowSidebar={setShowSidebar}
        gridSize={gridSize}
        setGridSize={setGridSize}
      />
      <div className="grow">
        <CamerasPage
          refreshAvailabilities={refreshAvailabilities}
          views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]}
          toggleSidebar={() => setShowSidebar(!showSidebar)}
          gridSize={gridSize}
        />
      </div>
    </div>
  )
}
