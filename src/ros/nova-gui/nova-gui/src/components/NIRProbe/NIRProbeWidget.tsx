import React, {useCallback, useState} from "react";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputWidget/NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "./NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeOutputWidget/NIRProbeFileTableWidget.tsx";
import NIRCalibrationCurveWidget from "./NIRProbeCalibration/NIRCalibrationCurveWidget.tsx";
import {useLocalStorage} from "../../hooks/useLocalStorage.ts";
import TOFHeight from "../AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import {DEFAULT_WATER_CALIBRATION} from "./NIRProbeCalibration/DefaultCalibrationPoints.ts";
import {useGenericStore} from "../../hooks/useGenericStore.ts";
import {Site} from "../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteData, SiteDataState} from "../../redux/models/genericStores/SiteDataState.ts";
import {SpaceResourcesSiteType} from "./SpaceResourcesSiteType.tsx";
import SiteTypeSelectWidget from "../SiteSelectWidget/SiteTypeSelectWidget.tsx";

interface INIRProbeWidgetProps {
}

const NIRProbeWidget: React.FC<INIRProbeWidgetProps> = () => {

  // current site as provided by the site selector
  const [currentSite, _] = useGenericStore<Site>("currentSite");

  // current space resource siteData for each site
  const [siteData, setSiteData] = useGenericStore<SiteDataState>("siteData");

  // whether to show the advanced capabilities
  const [showAdvanced, setShowAdvanced] = useState<boolean>(false);

  // set the current site's file to a new file
  const setFileForCurrentSite = useCallback((newFile: SiteData) => {
    setSiteData({...siteData, [currentSite]: newFile})
  }, [siteData, currentSite, setSiteData])


  // The currently selected site's file
  const file = siteData[currentSite];



  return (
    <div className="grid grid-flow-col auto-cols-fr gap-3">
      <div className="flex flex-col gap-3 col-span-3">
        <NIRProbeLEDWidget/>
        <NIRProbeOutputSaveWidget file={file} setFile={setFileForCurrentSite} showAdvanced={showAdvanced} setShowAdvanced={setShowAdvanced}/>
        <NIRCalibrationCurveWidget/>
      </div>
      <div className="flex flex-col gap-3 col-span-3">
        <TOFHeight/>
        <SiteTypeSelectWidget/>
        {/*<NIRProbeFileTableWidget showAdvanced={showAdvanced} absorbance={absorbance} calibrationFunction={calibrationFunction}></NIRProbeFileTableWidget>*/}
      </div>
    </div>
  );
}

export default NIRProbeWidget;
