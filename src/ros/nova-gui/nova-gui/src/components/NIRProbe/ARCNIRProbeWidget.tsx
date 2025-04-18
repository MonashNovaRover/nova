import React, {useState} from "react";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputWidget/NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "./NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTable/NIRProbeFileTableWidget.tsx";
import NIRCalibrationCurveWidget from "./NIRProbeCalibration/NIRCalibrationCurveWidget.tsx";
import TOFHeight from "../AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import SiteTypeSelectWidget from "../SiteSelectWidget/SiteTypeSelectWidget.tsx";
import NIRProbeCalcTable from "./NIRProbeFileTable/NIRProbeCalcTable.tsx";
import NIRCalibrationSettingsModal from "./NIRProbeFileTable/NIRCalibrationSettingsModal.tsx";
import {ARCNIRPRobeReadingTypeInfo} from "./SpaceResourcesSiteType.tsx";

interface INIRProbeWidgetProps {
}

/**
 * View of all NIR Probe widgets for ARCh
 * @constructor
 */
const ARCNIRProbeWidget: React.FC<INIRProbeWidgetProps> = () => {
  // whether to show the advanced capabilities
  const [showAdvanced, setShowAdvanced] = useState<boolean>(false);

  return (
    <div className="grid grid-flow-col auto-cols-fr gap-3">
      <div className="flex flex-col gap-3 col-span-3">
        <NIRProbeLEDWidget readingInfo={ARCNIRPRobeReadingTypeInfo}/>
        <NIRProbeOutputSaveWidget
          showAdvanced={showAdvanced}
          setShowAdvanced={setShowAdvanced}
          readingInfo={ARCNIRPRobeReadingTypeInfo}
        />
        <NIRCalibrationCurveWidget/>
      </div>
      <div className="flex flex-col gap-3 col-span-3">
        <TOFHeight/>
        <SiteTypeSelectWidget/>
        <NIRProbeFileTableWidget
          Modal={NIRCalibrationSettingsModal}
          headerTable={<NIRProbeCalcTable/>}
          readingInfo={ARCNIRPRobeReadingTypeInfo}
        />
      </div>
    </div>
  );
}

export default ARCNIRProbeWidget;
