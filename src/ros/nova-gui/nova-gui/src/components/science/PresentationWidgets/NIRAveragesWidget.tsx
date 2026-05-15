import React from "react";
import {Card, CardHeader, CardBody, Divider} from "@nextui-org/react";
import {useAverageReading} from "../NIRProbe/NIRProbeCalibration/NIRCalibration.ts";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {OverlayedProgress} from "../../shared/components/OverlayedProgress/OverlayedProgress.tsx";
import {useNIRSiteData} from "../NIRProbe/useNIRSiteData.ts";

const NIRAveragesWidget: React.FC = () => {
  const [currentSite, _] = useGenericStore<Site>("currentSite");
  const [averageX, averageY, calibratedResult] = useAverageReading();
  const [readings, __] = useNIRSiteData();

  const pd1Count = readings[1]?.length || 0;
  const pd2Count = readings[2]?.length || 0;
  const hasData = pd1Count > 0 || pd2Count > 0;

  return (
    <Card>
      <CardHeader className="text-h1 pb-0">
        NIR Probe Averages - Site {currentSite + 1}
      </CardHeader>
      <CardBody>
        {!hasData ? (
          <p className="text-default-500 text-center py-4">No NIR data collected</p>
        ) : (
          <>
            <div className="grid grid-cols-2 gap-4">
              <OverlayedProgress
                label={`PD1 Average (n=${pd1Count})`}
                value={averageX}
                maxValue={27000}
                autoColor
              >
                {averageX.toFixed(2)}
              </OverlayedProgress>

              <OverlayedProgress
                label={`PD2 Average (n=${pd2Count})`}
                value={averageY}
                maxValue={27000}
                autoColor
              >
                {averageY.toFixed(2)}
              </OverlayedProgress>
            </div>

            <Divider className="my-3" />

            <div className="text-center">
              <p className="text-small text-default-500">Calibrated Result</p>
              <p className="text-2xl font-bold">{calibratedResult.toFixed(3)}%</p>
            </div>
          </>
        )}
      </CardBody>
    </Card>
  );
};

export default NIRAveragesWidget;
