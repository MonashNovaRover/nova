import React, {useCallback, useState} from "react";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputWidget/NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "./NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeOutputWidget/NIRProbeFileTableWidget.tsx";
import NIRCalibrationCurveWidget, {EMPTY_CALIBRATION_DATA, NIRCalibrationData} from "./NIRProbeCalibration/NIRCalibrationCurveWidget.tsx";
import {useLocalStorage} from "../../hooks/useLocalStorage.ts";
import TOFHeight from "../AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import {DEFAULT_WATER_CALIBRATION} from "./NIRProbeCalibration/DefaultWaterCalibration.ts";
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

  const [calibrationData, setCalibrationData] = useLocalStorage<NIRCalibrationData>(
    file.siteType === SpaceResourcesSiteType.WATER ? "nir-calibration-water" : "nir-calibration-ilmenite",
    file.siteType === SpaceResourcesSiteType.WATER ? DEFAULT_WATER_CALIBRATION : EMPTY_CALIBRATION_DATA,
    [file.siteType]
  );

  // This function maps differences to predicted concentrations
  const calibrationFunction = useCallback((rawValue: number) => {
    return (rawValue - calibrationData.yIntercept) / calibrationData.gradient;
  }, [calibrationData.gradient, calibrationData.yIntercept])
  const maxCalibrationDifference = calibrationData.points.map(v => v.difference).reduce((a, b) => Math.max(a,b), 0);
  const absorbance = useCallback((rawDifference: number) => {
    return Math.log10(maxCalibrationDifference / rawDifference);
  }, [maxCalibrationDifference])

  return (
    <div className="grid grid-flow-col auto-cols-fr gap-3">
      <div className="flex flex-col gap-3 col-span-3">
        <NIRProbeLEDWidget/>
        <NIRProbeOutputSaveWidget file={file} setFile={setFileForCurrentSite} showAdvanced={showAdvanced} setShowAdvanced={setShowAdvanced}/>
        <NIRCalibrationCurveWidget files={siteData} type={file.siteType} absorbance={absorbance} calibrationFunction={calibrationFunction}
                                   calibrationData={calibrationData} setCalibrationData={setCalibrationData}/>
      </div>
      <div className="flex flex-col gap-3 col-span-3">
        <TOFHeight/>
        <SiteTypeSelectWidget/>
        <NIRProbeFileTableWidget file={file} setFile={setFileForCurrentSite} showAdvanced={showAdvanced} absorbance={absorbance} calibrationFunction={calibrationFunction}></NIRProbeFileTableWidget>
      </div>
    </div>
  );
}

export default NIRProbeWidget;
