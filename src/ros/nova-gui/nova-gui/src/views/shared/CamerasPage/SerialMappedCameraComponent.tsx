import {FC, memo, useMemo} from "react";
import {BaseCameraComponentProps, CameraComponent} from "../../../components/CameraComponent/CameraComponent.tsx";
import {CameraSerials} from "./CameraPageConstants.tsx";
import BarOverlayedCameraComponent from "../../../components/CameraComponent/special/BarOverlayedCameraComponent.tsx";
//import KeyboardOverlayedCameraComponent from "../../../components/CameraComponent/special/KeyboardOverlayedCameraComponent.tsx";
import {
  GimbalOverlayedCameraComponent
} from "../../../components/CameraComponent/special/GimbalOverlayedCameraComponent.tsx";
import DriveCameraComponent from "../../../components/CameraComponent/special/DriveCameraComponent.tsx";
import WheelTelemetryCameraComponent
  from "../../../components/CameraComponent/special/WheelTelemetryCameraComponent.tsx";
import SiteSelectCameraComponent from "../../../components/CameraComponent/special/SiteSelectCameraComponent.tsx";

/// Defines special components to use for certain cameras
export const cameraSerialToComponentMap: { [k: string]: FC<BaseCameraComponentProps> } = {
  [CameraSerials.ARM_END_PERISCOPE]: BarOverlayedCameraComponent,
  [CameraSerials.SCIENCE_GIMBAL]: GimbalOverlayedCameraComponent,
  [CameraSerials.WHEEL_TELEMETRY]: WheelTelemetryCameraComponent,
  [CameraSerials.DRIVE_TELEMETRY]: DriveCameraComponent,
  [CameraSerials.SITE_SELECT]: SiteSelectCameraComponent,
}

/// Function that used the above map to get the component for a specified camera serial
const cameraSerialToComponent: (serial: string) => FC<BaseCameraComponentProps> = (serial) => {
  if (serial in cameraSerialToComponentMap)
    return cameraSerialToComponentMap[serial];
  return CameraComponent;
}

const SerialMappedCameraComponentUnmemoed: FC<BaseCameraComponentProps> = (props) => {
  const SerialMappedComponent = useMemo(() => cameraSerialToComponent(props.cameraSerial), [props.cameraSerial]);
  return <SerialMappedComponent {...props}></SerialMappedComponent>
}

const SerialMappedCameraComponent = memo(SerialMappedCameraComponentUnmemoed);
export default SerialMappedCameraComponent;
