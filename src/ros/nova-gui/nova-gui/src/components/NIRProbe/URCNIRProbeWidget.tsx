import React, {useState} from "react";
import NIRProbeOutputSaveWidget from "./NIRProbeOutputWidget/NIRProbeOutputSaveWidget.tsx";
import NIRProbeLEDWidget from "./NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeFileTableWidget from "./NIRProbeFileTable/NIRProbeFileTableWidget.tsx";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";
import NIRProbeSiteAvgTable from "./NIRProbeFileTable/NIRProbeSiteAvgTable.tsx";
import {URCNIRPRobeReadingTypeInfo} from "./SpaceResourcesSiteType.tsx";

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
      <NIRProbeLEDWidget readingInfo={URCNIRPRobeReadingTypeInfo}/>
      <NIRProbeOutputSaveWidget
        showAdvanced={showAdvanced}
        setShowAdvanced={setShowAdvanced}
        readingInfo={URCNIRPRobeReadingTypeInfo}
      />
      <SiteSelectWidget/>
      <NIRProbeFileTableWidget
        headerTable={<NIRProbeSiteAvgTable/>}
        readingInfo={URCNIRPRobeReadingTypeInfo}
      />
    </div>
  );
}

export default URCNIRProbeWidget;
