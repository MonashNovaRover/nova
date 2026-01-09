import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import {CircularProgress} from "@nextui-org/react";
import {min} from "lodash";


export const CamerasCircularProgress = () => {
  const onlineCameras = useSelector(
    (state: RootState) => state.camerasStore.cameras
  );

  const onlineCameraSerials = onlineCameras.map((cam) => cam.serial);

  const cameraStreamerMap = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  const streamingCameras = onlineCameraSerials.filter(v => !!cameraStreamerMap[v])

  return (
    <CircularProgress
      maxValue={min([onlineCameraSerials.length, 8])}
      value={streamingCameras.length}
      color={"success"}
      showValueLabel
    />
  )
}
