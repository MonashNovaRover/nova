
import React, {useCallback, useEffect, useState} from "react";
import SiteSelectWidget, {siteFilenames} from "../../SiteSelectWidget/SiteSelectWidget.tsx";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "../NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTableWidget.tsx";
import NIRCalibrationCurveWidget from "./NIRCalibrationCurveWidget.tsx";
import SpaceResourceSiteType from "../SpaceResourcesSiteType.tsx";




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
  }, [filename]);

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

  return (
    <div className="grid grid-flow-col auto-cols-fr gap-3">
      <div className="flex flex-col gap-3 col-span-3">
        <NIRProbeLEDWidget/>
        <NIRProbeOutputSaveWidget file={file} setFile={setFileAndSave} showAdvanced={showAdvanced} setShowAdvanced={setShowAdvanced}/>
        <NIRCalibrationCurveWidget files={files} type={file.type}/>
      </div>
      <div className="flex flex-col gap-3 col-span-2">
        <SiteSelectWidget onValueChanged={setFilename}
                          onSiteTypeChanged={onSiteTypeChanged}
                          currentSiteType={file.type}/>
        <NIRProbeFileTableWidget file={file} setFile={setFileAndSave} showAdvanced={showAdvanced}></NIRProbeFileTableWidget>
      </div>
    </div>
  );
}

export default NIRProbeWidget;

