import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import {CircularProgress} from "@nextui-org/react";
import {useEffect} from "react";

export const CamerasCircularProgress = () => {
  const onlineCameras = useSelector(
    (state: RootState) => state.camerasStore.cameras
  );

  const onlineCameraSerials = onlineCameras.map((cam) => cam.serial);

  const cameraStreamerMap = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  const streamingCameras = onlineCameraSerials.filter(v => !!cameraStreamerMap[v])

  useEffect(() => {
    console.log(cameraStreamerMap)
  }, [cameraStreamerMap]);

  useEffect(() => {
    console.log(onlineCameras)
  }, [onlineCameras]);

  return (
    <CircularProgress
      maxValue={onlineCameraSerials.length}
      value={streamingCameras.length}
      valueLabel={`${streamingCameras.length}/${onlineCameraSerials.length}`}
      color={"success"} // filled part
      classNames={{
        track: onlineCameraSerials.length > 0 ? "stroke-primary" : "stroke-default-300",   // empty part
      }}
      showValueLabel
    />
  )
}
