import React, {useCallback, useEffect, useState} from "react";
import SiteSelectWidget from "../../SiteSelectWidget/SiteSelectWidget.tsx";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "../NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTableWidget.tsx";
import NIRCalibrationCurveWidget, {EMPTY_CALIBRATION_DATA, NIRCalibrationData} from "./NIRCalibrationCurveWidget.tsx";
import SpaceResourceSiteType from "../SpaceResourcesSiteType.tsx";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import TOFHeight from "../../AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import {DEFAULT_WATER_CALIBRATION} from "./DefaultWaterCalibration.ts";
import {useGenericStore} from "../../../redux/actions/useGenericStore.ts";
import {CurrentSiteStore} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {ISpaceResourcesFile, NIRProbeFilesState} from "../../../redux/models/genericStores/NIRProbeFilesState.ts";

interface INIRProbeWidgetProps {
}

const NIRProbeWidget: React.FC<INIRProbeWidgetProps> = () => {

  // current site as provided by the site selector
  const [currentSite, _] = useGenericStore<CurrentSiteStore>("currentSite");

  // current space resource files for each site
  const [files, setFiles] = useGenericStore<NIRProbeFilesState>("NIRProbeFiles");

  // whether to show the advanced capabilities
  const [showAdvanced, setShowAdvanced] = useState<boolean>(false);

  // set the current site's file to a new file
  const setFileForCurrentSite = useCallback((newFile: ISpaceResourcesFile) => {
    setFiles({...files, [currentSite.site]: newFile})
  }, [setFiles, files, currentSite.site])

  // change the type of the current site's file
  useEffect(() => {
    setFileForCurrentSite({...files[currentSite.site], type: currentSite.type})
  }, [currentSite.type]);

  // The currently selected site's file
  const file = files[currentSite.site];

  const [calibrationData, setCalibrationData] = useLocalStorage<NIRCalibrationData>(
    file.type === SpaceResourceSiteType.WATER ? "nir-calibration-water" : "nir-calibration-ilmenite",
    file.type === SpaceResourceSiteType.WATER ? DEFAULT_WATER_CALIBRATION : EMPTY_CALIBRATION_DATA,
    [file.type]
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
        <NIRCalibrationCurveWidget files={files} type={file.type} absorbance={absorbance} calibrationFunction={calibrationFunction}
                                   calibrationData={calibrationData} setCalibrationData={setCalibrationData}/>
      </div>
      <div className="flex flex-col gap-3 col-span-3">
        <TOFHeight/>
        <SiteSelectWidget/>
        <NIRProbeFileTableWidget file={file} setFile={setFileForCurrentSite} showAdvanced={showAdvanced} absorbance={absorbance} calibrationFunction={calibrationFunction}></NIRProbeFileTableWidget>
      </div>
    </div>
  );
}

export default NIRProbeWidget;
