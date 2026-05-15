import React, {useMemo} from "react";
import {Card, CardHeader, CardBody, Divider} from "@nextui-org/react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";
import {NIRProbeReadingType} from "../NIRProbe/SpaceResourcesSiteType.tsx";
import {calibrationFunction} from "../NIRProbe/NIRProbeCalibration/NIRCalibration.ts";
import {NIRProbeCalibrationData} from "../../../redux/models/genericStores/NIRProbeCalibrationData.ts";

interface SensorDataWidgetProps {
  site: Site;
}

const SensorDataWidget: React.FC<SensorDataWidgetProps> = ({site}) => {
  const [siteData, _] = useGenericStore<SiteDataState>("siteData");
  const [calibrationData, __] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  const sensorData = useMemo(() => siteData[site].sensorData, [siteData, site]);

  // Get NIR data for this site
  const readings = useMemo(() => siteData[site].spaceResourcesEntries, [siteData, site]);
  const pd1Count = readings[NIRProbeReadingType.PD1]?.length || 0;
  const pd2Count = readings[NIRProbeReadingType.PD2]?.length || 0;
  const hasNIRData = pd1Count > 0 || pd2Count > 0;

  // Calculate NIR averages
  const averageX = useMemo(() => {
    const xList = readings[NIRProbeReadingType.PD1]?.map(entry => entry.data) || [];
    return xList.length > 0 ? xList.reduce((a, b) => a + b, 0) / xList.length : 0;
  }, [readings]);

  const averageY = useMemo(() => {
    const yList = readings[NIRProbeReadingType.PD2]?.map(entry => entry.data) || [];
    return yList.length > 0 ? yList.reduce((a, b) => a + b, 0) / yList.length : 0;
  }, [readings]);

  const calibratedResult = useMemo(() => {
    const calibFunc = calibrationFunction(calibrationData.coefficients);
    return calibFunc(averageX, averageY);
  }, [averageX, averageY, calibrationData]);

  return (
    <Card>
      <CardHeader className="text-h1 pb-0">
        Sensor Data - Site {site + 1}
      </CardHeader>
      <CardBody className="gap-4">
        {/* Sensor Data Section */}
        <div>
          {sensorData.length === 0 ? (
            <p className="text-default-500 text-center py-4">No sensor data saved</p>
          ) : (
            <div className="grid grid-cols-5 gap-3">
              {sensorData.map((sensor, index) => (
                <div key={index} className="flex flex-col">
                  <span className="text-small text-default-500">{sensor.name}</span>
                  <span className="text-lg font-semibold">{sensor.data.toFixed(2)}</span>
                </div>
              ))}
            </div>
          )}
        </div>

        <Divider />

        {/* NIR Averages Section */}
        <div>
          <h3 className="text-medium font-semibold mb-2">NIR Probe Averages</h3>
          {!hasNIRData ? (
            <p className="text-default-500 text-center py-2">No NIR data collected</p>
          ) : (
            <div className="grid grid-cols-3 gap-3">
              <div className="flex flex-col">
                <span className="text-small text-default-500">PD1 Average (n={pd1Count})</span>
                <span className="text-lg font-semibold">{averageX.toFixed(2)}</span>
              </div>
              <div className="flex flex-col">
                <span className="text-small text-default-500">PD2 Average (n={pd2Count})</span>
                <span className="text-lg font-semibold">{averageY.toFixed(2)}</span>
              </div>
              <div className="flex flex-col">
                <span className="text-small text-default-500">Calibrated Result</span>
                <span className="text-lg font-semibold">{calibratedResult.toFixed(3)}%</span>
              </div>
            </div>
          )}
        </div>
      </CardBody>
    </Card>
  );
};

export default SensorDataWidget;
