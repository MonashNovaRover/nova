import {CameraComponent} from "../../../components/CameraComponent/CameraComponent.tsx";
import {CameraSerials} from "../../shared/CamerasPage/CameraPageConstants.ts";

export default function TestScimbalCamView () {

  return (
    <div>
      <CameraComponent cameraSerial={CameraSerials.SCIENCE_GIMBAL}/>
    </div>
  );
}