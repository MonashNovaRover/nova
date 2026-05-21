import React, {useMemo} from "react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";
import {NIRProbeReadingType} from "../NIRProbe/SpaceResourcesSiteType.tsx";
import {getThymineResult, getAmmoniaResult, getPD1Label, getPD2Label} from "../NIRProbe/nirThresholdConfig.ts";

interface NIRResultsWidgetProps {
  site: Site;
}

const NIRResultsWidget: React.FC<NIRResultsWidgetProps> = ({site}) => {
  const [siteData, _] = useGenericStore<SiteDataState>("siteData");

  // Get NIR data for this site
  const readings = useMemo(() => siteData[site].spaceResourcesEntries, [siteData, site]);
  const pd1Count = readings[NIRProbeReadingType.PD1]?.length || 0;
  const pd2Count = readings[NIRProbeReadingType.PD2]?.length || 0;
  const hasNIRData = pd1Count > 0 || pd2Count > 0;

  // Calculate NIR averages
  const averagePD1 = useMemo(() => {
    const xList = readings[NIRProbeReadingType.PD1]?.map(entry => entry.data) || [];
    return xList.length > 0 ? xList.reduce((a, b) => a + b, 0) / xList.length : 0;
  }, [readings]);

  const averagePD2 = useMemo(() => {
    const yList = readings[NIRProbeReadingType.PD2]?.map(entry => entry.data) || [];
    return yList.length > 0 ? yList.reduce((a, b) => a + b, 0) / yList.length : 0;
  }, [readings]);

  // Get detection results using threshold logic
  const thymineResult = useMemo(() => getThymineResult(averagePD1), [averagePD1]);
  const ammoniaResult = useMemo(() => getAmmoniaResult(averagePD2), [averagePD2]);

  return (
    <div className="rounded-lg border-2 border-[#3eb1cf]/30 bg-[#3eb1cf]/5 p-3">
      <h3 className="text-medium font-semibold mb-3 text-[#3eb1cf]">NIR Probe Results</h3>
      {!hasNIRData ? (
        <p className="text-default-500 text-center py-2">No NIR data collected</p>
      ) : (
        <div className="flex flex-row gap-6 items-center">
          <div className="flex flex-row items-center gap-9 flex-1">
            <div className="flex flex-col">
              <span className="text-small text-primar-500">{getPD1Label()} (n={pd1Count})</span>
              <span className="text-xs text-default-400">Avg: {averagePD1.toFixed(2)}</span>
            </div>
            <span className={`text-lg font-semibold ${thymineResult.detected ? 'text-[#3eb1cf]' : 'text-default-500'}`}>
              {thymineResult.displayText}
            </span>
          </div>
          <div className="flex flex-row items-center gap-9 flex-1">
            <div className="flex flex-col">
              <span className="text-small text-default-500">{getPD2Label()} (n={pd2Count})</span>
              <span className="text-xs text-default-400">Avg: {averagePD2.toFixed(2)}</span>
            </div>
            <span className={`text-lg font-semibold ${ammoniaResult.detected ? 'text-[#3eb1cf]' : 'text-default-500'}`}>
              {ammoniaResult.displayText}
            </span>
          </div>
        </div>
      )}
    </div>
  );
};

export default NIRResultsWidget;
