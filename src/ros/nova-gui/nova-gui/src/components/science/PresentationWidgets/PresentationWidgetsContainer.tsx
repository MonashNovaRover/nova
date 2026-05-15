import React from "react";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";
import NIRAveragesWidget from "./NIRAveragesWidget.tsx";
import SensorDataWidget from "./SensorDataWidget.tsx";
import UVVisComparisonWidget from "./UVVisComparisonWidget.tsx";

const PresentationWidgetsContainer: React.FC = () => {
  return (
    <div className="flex flex-col gap-3 p-3 overflow-auto">
      <SiteSelectWidget numSites={2} />

      <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-3">
        <NIRAveragesWidget />
        <SensorDataWidget />
        <UVVisComparisonWidget />
      </div>
    </div>
  );
};

export default PresentationWidgetsContainer;
