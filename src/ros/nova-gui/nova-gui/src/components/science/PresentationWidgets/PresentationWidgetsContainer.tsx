import React from "react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import SensorDataWidget from "./SensorDataWidget.tsx";
import ChemicalComparisonWidget from "./ChemicalComparisonWidget.tsx";
import PotentiostatWidget from "./PotentiostatWidget.tsx";

const PresentationWidgetsContainer: React.FC = () => {
  return (
    <div className="grid grid-cols-2 gap-3 p-3 h-full overflow-auto">
      {/* Site 1 Column */}
      <div className="flex flex-col gap-3">
        <div className="flex flex-row gap-3">
          <div className="flex-[3]">
            <SensorDataWidget site={Site.SITE_1} />
          </div>
          <div className="flex-[2]">
            <PotentiostatWidget site={Site.SITE_1} />
          </div>
        </div>
        <ChemicalComparisonWidget site={Site.SITE_1} />
      </div>

      {/* Site 2 Column */}
      <div className="flex flex-col gap-3">
        <div className="flex flex-row gap-3">
          <div className="flex-[3]">
            <SensorDataWidget site={Site.SITE_2} />
          </div>
          <div className="flex-[2]">
            <PotentiostatWidget site={Site.SITE_2} />
          </div>
        </div>
        <ChemicalComparisonWidget site={Site.SITE_2} />
      </div>
    </div>
  );
};

export default PresentationWidgetsContainer;
