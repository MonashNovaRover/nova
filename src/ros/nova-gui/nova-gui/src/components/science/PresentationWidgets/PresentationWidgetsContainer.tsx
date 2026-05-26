import React, {useState} from "react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import SensorDataWidget from "./SensorDataWidget.tsx";
import ChemicalComparisonWidget, {Spectrometer} from "./ChemicalComparisonWidget.tsx";
import PotentiostatWidget from "./PotentiostatWidget.tsx";

const PresentationWidgetsContainer: React.FC = () => {
  // State for spectrometer column mapping
  // false = default (Left: SL, Right: SR), true = swapped (Left: SR, Right: SL)
  const [isSwapped, setIsSwapped] = useState(false);

  // Shared chemical selection state for both comparison widgets
  const [selectedChemicalIndex, setSelectedChemicalIndex] = useState(0);

  const leftSpectrometer: Spectrometer = isSwapped ? "SR" : "SL";
  const rightSpectrometer: Spectrometer = isSwapped ? "SL" : "SR";

  const handleSwap = () => {
    setIsSwapped(!isSwapped);
  };

  return (
    <div className="grid grid-cols-2 gap-3 p-3 h-full overflow-auto">
      {/* Left Column (Site 1) */}
      <div className="flex flex-col gap-3">
        <div className="flex flex-row gap-3">
          <div className="flex-[3]">
            <SensorDataWidget site={Site.SITE_1} />
          </div>
          <div className="flex-[2]">
            <PotentiostatWidget site={Site.SITE_1} />
          </div>
        </div>
        <ChemicalComparisonWidget
          spectrometer={leftSpectrometer}
          columnLabel={`Site 1 (${leftSpectrometer})`}
          onSwap={handleSwap}
          currentMapping={`Site 1: ${leftSpectrometer}`}
          selectedIndex={selectedChemicalIndex}
          onIndexChange={setSelectedChemicalIndex}
        />
      </div>

      {/* Right Column (Site 2) */}
      <div className="flex flex-col gap-3">
        <div className="flex flex-row gap-3">
          <div className="flex-[3]">
            <SensorDataWidget site={Site.SITE_2} />
          </div>
          <div className="flex-[2]">
            <PotentiostatWidget site={Site.SITE_2} />
          </div>
        </div>
        <ChemicalComparisonWidget
          spectrometer={rightSpectrometer}
          columnLabel={`Site 2 (${rightSpectrometer})`}
          onSwap={handleSwap}
          currentMapping={`Site 2: ${rightSpectrometer}`}
          selectedIndex={selectedChemicalIndex}
          onIndexChange={setSelectedChemicalIndex}
        />
      </div>
    </div>
  );
};

export default PresentationWidgetsContainer;
