import { useCameraStreamer } from "../../../components/cameras/hooks/useCameraStreamer";
import {CameraViewConfig} from "./CameraViewConstants.tsx";
import { useState } from "react";
import {CameraSidebar} from "../../../components/cameras/CameraPage/CameraSidebar.tsx";
import {CamerasPage} from "../../../components/cameras/CameraPage/CamerasPage.tsx";
import {defaultCameraProfilePresets} from "./CameraProfileConstants.ts";

export interface CameraViewProps {
  views: CameraViewConfig[];
  defaultGridSize?: number;
}

export const CameraView = (props: CameraViewProps) => {
    const { refreshAvailabilities } = useCameraStreamer();
    const [showSidebar, setShowSidebar] = useState(false)
    const [gridSize, setGridSize] = useState(props.defaultGridSize ? props.defaultGridSize : 4)

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
          presets={props.views[0].cameraPrests ? props.views[0].cameraPrests : defaultCameraProfilePresets}
          serialPresetGroups={props.views[0].serialPresetGroups}
        />
        <div className="grow">
          <CamerasPage
            refreshAvailabilities={refreshAvailabilities}
            views={props.views}
            toggleSidebar={() => setShowSidebar(!showSidebar)}
            gridSize={gridSize}
          />
        </div>
      </div>
    )
  }
