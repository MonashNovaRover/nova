import React, {useState} from "react";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputWidget/NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "./NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTable/NIRProbeFileTableWidget.tsx";
import NIRCalibrationCurveWidget from "./NIRProbeCalibration/NIRCalibrationCurveWidget.tsx";
import TOFHeight from "../AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import SiteTypeSelectWidget from "../SiteSelectWidget/SiteTypeSelectWidget.tsx";

interface INIRProbeWidgetProps {
}

/**
 * View of all NIR Probe widgets
 * @constructor
 */
const NIRProbeWidget: React.FC<INIRProbeWidgetProps> = () => {
  // whether to show the advanced capabilities
  const [showAdvanced, setShowAdvanced] = useState<boolean>(false);

  return (
    <div className="grid grid-flow-col auto-cols-fr gap-3">
      <div className="flex flex-col gap-3 col-span-3">
        <NIRProbeLEDWidget/>
        <NIRProbeOutputSaveWidget showAdvanced={showAdvanced} setShowAdvanced={setShowAdvanced}/>
        <NIRCalibrationCurveWidget/>
      </div>
      <div className="flex flex-col gap-3 col-span-3">
        <TOFHeight/>
        <SiteTypeSelectWidget/>
        <NIRProbeFileTableWidget/>
      </div>
    </div>
  );
}

export default NIRProbeWidget;
