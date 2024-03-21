
import React, {useCallback, useEffect, useState} from "react";
import SiteSelectWidget, {siteFilenames} from "../../SiteSelectWidget/SiteSelectWidget.tsx";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "../NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTableWidget.tsx";
import NIRCalibrationCurveWidget, {EMPTY_CALIBRATION_DATA, NIRCalibrationData} from "./NIRCalibrationCurveWidget.tsx";
import SpaceResourceSiteType from "../SpaceResourcesSiteType.tsx";
import {useLocalStorage} from "../hooks/useLocalStorage.ts";
import AnalysisPlatformHeight from "../../AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";




interface INIRProbeWidgetProps {

}




interface ISpaceResourcesEntry {
  lightBlank?: number,
  difference: number,
  concentration?: number,
  label: string
}


export interface ISpaceResourcesFile {
  entries: ISpaceResourcesEntry[],
  type: SpaceResourceSiteType
}
const EMPTY_SPACE_RESOURCES_FILE: ISpaceResourcesFile = {entries: [], type: SpaceResourceSiteType.WATER};

// TODO: find a better solution for this
const DEFAULT_WATER_CALIBRATION = {"points":[{"difference":3029.5,"concentration":0},{"difference":2245.666667,"concentration":2.5},{"difference":2663.666667,"concentration":5},{"difference":2311.833333,"concentration":7.5},{"difference":1638,"concentration":10},{"difference":1262.333333,"concentration":12.5},{"difference":1494.5,"concentration":15},{"difference":1247,"concentration":17.5},{"difference":1304.666667,"concentration":20},{"difference":886.5,"concentration":22.5},{"difference":987,"concentration":25},{"difference":571.3333333,"concentration":30}],"yIntercept":0.00875,"gradient":0.0218,"chemBlankDifference":2155.833333};


const NIRProbeWidget: React.FC<INIRProbeWidgetProps> = () => {

  const [files, setFiles] = useState<{[key: string] : ISpaceResourcesFile}>({});

  const [filename, setFilename] = useState<string | undefined>();
  const [showAdvanced, setShowAdvanced] = useState<boolean>(false);

  const setFileAndSave = useCallback((newFile: ISpaceResourcesFile) => {
    if (!filename)
      return;

    const newFiles = {...files};
    newFiles[filename] = newFile;

    setFiles(newFiles);
    localStorage.setItem(filename, JSON.stringify(newFile));
  }, [files, filename])

  const onSiteTypeChanged = useCallback((newType: SpaceResourceSiteType) => {
    if (filename === undefined)
      return;

    const newFile = {...files[filename], type: newType};
    setFileAndSave(newFile);
  }, [filename, files, setFileAndSave]);

  useEffect(() => {
    // Load every site's file from local storage
    const newFiles = Object.fromEntries(
      siteFilenames
        .map((name) => {
          const storedFileJSON = localStorage.getItem(name);
          if (storedFileJSON === null)
            return [name, EMPTY_SPACE_RESOURCES_FILE];

          // Parse JSON and do some checks to make sure it is the correct type
          const storedFile = JSON.parse(storedFileJSON)
          if (storedFile === undefined || storedFile.entries === undefined || storedFile.type === undefined)
            return [name, EMPTY_SPACE_RESOURCES_FILE];

          // This value was successfully retrieved from local storage.
          return [name, storedFile]
        })
    );

    console.log(newFiles);
    setFiles(newFiles);
  }, []);

  // The currently selected site's file
  const file = filename === undefined ? EMPTY_SPACE_RESOURCES_FILE : files[filename];

  const [calibrationData, setCalibrationData] = useLocalStorage<NIRCalibrationData>(
    file.type === SpaceResourceSiteType.WATER ? "nir-calibration-water" : "nir-calibration-ilmenite",
    file.type === SpaceResourceSiteType.WATER ? DEFAULT_WATER_CALIBRATION : EMPTY_CALIBRATION_DATA,
    [file.type]
  );


  // This function maps differences to predicted concentrations
  const calibrationFunction = useCallback((rawValue: number) => {
    return (rawValue - calibrationData.yIntercept) / calibrationData.gradient ;
  }, [calibrationData.gradient, calibrationData.yIntercept])
  const maxCalibrationDifference = calibrationData.points.map(v => v.difference).reduce((a, b) => Math.max(a,b), 0);
  const absorbance = useCallback((rawDifference: number) => {
    return Math.log10(maxCalibrationDifference / rawDifference);
  }, [maxCalibrationDifference])

  return (
    <div className="grid grid-flow-col auto-cols-fr gap-3">
      <div className="flex flex-col gap-3 col-span-3">
        <NIRProbeLEDWidget/>
        <NIRProbeOutputSaveWidget file={file} setFile={setFileAndSave} showAdvanced={showAdvanced} setShowAdvanced={setShowAdvanced}/>
        <NIRCalibrationCurveWidget files={files} type={file.type} absorbance={absorbance} calibrationFunction={calibrationFunction}
                                   calibrationData={calibrationData} setCalibrationData={setCalibrationData}/>
      </div>
      <div className="flex flex-col gap-3 col-span-3">
        <AnalysisPlatformHeight/>

        <SiteSelectWidget onValueChanged={setFilename}
                          onSiteTypeChanged={onSiteTypeChanged}
                          currentSiteType={file.type}/>
        <NIRProbeFileTableWidget file={file} setFile={setFileAndSave} showAdvanced={showAdvanced} absorbance={absorbance} calibrationFunction={calibrationFunction}></NIRProbeFileTableWidget>
      </div>
    </div>
  );
}

export default NIRProbeWidget;

