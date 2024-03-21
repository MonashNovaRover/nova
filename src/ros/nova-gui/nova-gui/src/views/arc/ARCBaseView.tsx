import React from "react";
import MicroscopeComponent from "../../components/CameraComponent/MicroscopeComponent";
import AnalysisPlatformHeight from "../../components/AnalysisPlatformHeight/AnalysisPlatformHeight";

const ARCBaseView: React.FC = () => {
  return (
    <div>
      <MicroscopeComponent/>
      <AnalysisPlatformHeight />
    </div>
  );
};

export default ARCBaseView;
