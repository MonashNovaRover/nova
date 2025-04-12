import React, {useState} from "react";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputWidget/NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "./NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTable/NIRProbeFileTableWidget.tsx";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";
import NIRProbeSiteAvgTable from "./NIRProbeFileTable/NIRProbeSiteAvgTable.tsx";

interface INIRProbeWidgetProps {
}

/**
 * View of all NIR Probe widgets
 * @constructor
 */
const URCNIRProbeWidget: React.FC<INIRProbeWidgetProps> = () => {
  // whether to show the advanced capabilities
  const [showAdvanced, setShowAdvanced] = useState<boolean>(false);

  return (
    <div className="flex flex-col gap-3 col-span-2 row-span-5">
      <NIRProbeLEDWidget/>
      <NIRProbeOutputSaveWidget showAdvanced={showAdvanced} setShowAdvanced={setShowAdvanced}/>
      <SiteSelectWidget/>
      <NIRProbeFileTableWidget headerTable={<NIRProbeSiteAvgTable/>}/>
    </div>
  );
}

export default URCNIRProbeWidget;
