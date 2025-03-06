import {FC, memo} from "react";
import {BaseCameraComponentProps, CameraComponent} from "../../../components/CameraComponent/CameraComponent.tsx";
import {CameraSerials} from "./CameraPageConstants.tsx";
import BarOverlayedCameraComponent from "../../../components/CameraComponent/special/BarOverlayedCameraComponent.tsx";

/// Defines special components to use for certain cameras
export const cameraSerialToComponentMap: { [k: string]: FC<BaseCameraComponentProps> } = {
  [CameraSerials.ARM_END_PERISCOPE]: BarOverlayedCameraComponent
}

/// Function that used the above map to get the component for a specified camera serial
export const cameraSerialToComponent: (serial: string) => FC<BaseCameraComponentProps> = (serial) => {
  if (serial in cameraSerialToComponentMap)
    return cameraSerialToComponentMap[serial];
  return CameraComponent;
}

const SerialMappedCameraComponentUnmemoed: FC<BaseCameraComponentProps> = (props) => {
  const component = cameraSerialToComponent(props.cameraSerial);
  return component(props);
}

const SerialMappedCameraComponent = memo(SerialMappedCameraComponentUnmemoed);
export default SerialMappedCameraComponent;
