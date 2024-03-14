
import React, {useCallback, useEffect, useState} from "react";
import SiteSelectWidget from "../../SiteSelectWidget/SiteSelectWidget.tsx";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "../NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTableWidget.tsx";



interface INIRProbeWidgetProps {

}


interface ISpaceResourcesEntry {
  lightBlank?: number,
  difference: number,
  concentration?: number,
  label: string
}

export interface ISpaceResourcesFile {
  entries: ISpaceResourcesEntry[]
}


const NIRProbeWidget: React.FC<INIRProbeWidgetProps> = () => {

  //const led = useSelector((state: RootState) => state.nirStore.led);

  const [file, setFile] = useState<ISpaceResourcesFile>({entries: []})
  const [filename, setFilename] = useState<string | undefined>();

  const setFileAndSave = useCallback((newFile: ISpaceResourcesFile) => {
    setFile(newFile);
    if (filename)
      localStorage.setItem(filename, JSON.stringify(newFile));
  }, [filename])

  useEffect(() => {
    if (!filename)
      return;

    const storedFile = localStorage.getItem(filename);
    if (storedFile === null)
      return;

    setFile(JSON.parse(storedFile));
  }, [filename]);

  return (
    <div className="flex flex-col gap-3">

      <SiteSelectWidget onValueChanged={setFilename}/>

      <NIRProbeLEDWidget/>
      <NIRProbeOutputSaveWidget file={file} setFile={setFileAndSave}>
      </NIRProbeOutputSaveWidget>
      <NIRProbeFileTableWidget file={file} setFile={setFileAndSave}></NIRProbeFileTableWidget>
    </div>
  );
}

export default NIRProbeWidget;

