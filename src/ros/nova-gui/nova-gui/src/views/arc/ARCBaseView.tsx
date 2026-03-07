import React from "react";
import MicroscopeComponent from "../../components/cameras/CameraComponent/MicroscopeComponent";
import TOFHeight from "../../components/science/AnalysisPlatformHeight/TOFHeight.tsx";

const ARCBaseView: React.FC = () => {
  return (
    <div>
      <MicroscopeComponent/>
      <TOFHeight />
    </div>
  );
};

export default ARCBaseView;
