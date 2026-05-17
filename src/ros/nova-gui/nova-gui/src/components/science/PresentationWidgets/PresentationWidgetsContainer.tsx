import React from "react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import SensorDataWidget from "./SensorDataWidget.tsx";
import ChemicalComparisonWidget from "./ChemicalComparisonWidget.tsx";

const PresentationWidgetsContainer: React.FC = () => {
  return (
    <div className="grid grid-cols-2 gap-3 p-3 h-full overflow-auto">
      {/* Site 1 Column */}
      <div className="flex flex-col gap-3">
        <SensorDataWidget site={Site.SITE_1} />
        <ChemicalComparisonWidget site={Site.SITE_1} />
      </div>

      {/* Site 2 Column */}
      <div className="flex flex-col gap-3">
        <SensorDataWidget site={Site.SITE_2} />
        <ChemicalComparisonWidget site={Site.SITE_2} />
      </div>
    </div>
  );
};

export default PresentationWidgetsContainer;
