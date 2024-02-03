import React from "react";
import { MicroscopeComponent } from "../../components/CameraComponent/MicroscopeComponent";

const ARCBaseView: React.FC = () => {
  return (
    <div>
      <MicroscopeComponent cameraName="stuff" cameraSerial="9"/>
    </div>
  );
};

export default ARCBaseView;
