import React from "react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import SensorDataWidget from "./SensorDataWidget.tsx";
import DualGraphWidget from "./DualGraphWidget.tsx";

const PresentationWidgetsContainer: React.FC = () => {
  return (
    <div className="grid grid-cols-2 gap-3 p-3 h-full overflow-auto">
      {/* Site 1 Column */}
      <div className="flex flex-col gap-3">
        <SensorDataWidget site={Site.SITE_1} />
        <DualGraphWidget site={Site.SITE_1} title="UV-Vis Spectra 1 - Site 1" />
        <DualGraphWidget site={Site.SITE_1} title="UV-Vis Spectra 2 - Site 1" />
      </div>

      {/* Site 2 Column */}
      <div className="flex flex-col gap-3">
        <SensorDataWidget site={Site.SITE_2} />
        <DualGraphWidget site={Site.SITE_2} title="UV-Vis Spectra 1 - Site 2" />
        <DualGraphWidget site={Site.SITE_2} title="UV-Vis Spectra 2 - Site 2" />
      </div>
    </div>
  );
};

export default PresentationWidgetsContainer;
