import React, {useMemo} from "react";
import {Card, CardHeader, CardBody} from "@nextui-org/react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";
import NIRResultsWidget from "./NIRResultsWidget.tsx";
import {useDisplayMapCoordinate} from "../../maps/Cartographer/utils/convertCoords.ts";

interface SensorDataWidgetProps {
  site: Site;
}

// HARDCODED SENSOR NAMES FOR URC 2026 COMPETITION PRESENTATION
// Group sensors by type:
// - Site Information: BME sensor (Temperature, Pressure) + GPS (Latitude, Longitude, Altitude)
// - Soil Information: Hydraprobe (Temperature, Moisture, Conductivity, Salinity)
const SensorDataWidget: React.FC<SensorDataWidgetProps> = ({site}) => {
  const [siteData, _] = useGenericStore<SiteDataState>("siteData");

  const sensorData = useMemo(() => siteData[site].sensorData, [siteData, site]);

  // Helper to get sensor value by name
  const getSensorValue = (name: string) => {
    const sensor = sensorData.find(s => s.name === name);
    if (!sensor) return "N/A";
    // Use higher precision for lat/long coordinates
    if (name === "Latitude" || name === "Longitude") {
      return sensor.data;
    }
    return sensor.data.toFixed(2);
  };

  const {lat: sensorLat, long: sensorLong} = useDisplayMapCoordinate({lat: +getSensorValue("Latitude"), long: +getSensorValue("Longitude")})

  return (
    <Card>
      <CardHeader className="text-h1 pb-0">
        Site {site + 1} Sensor Data
      </CardHeader>
      <CardBody className="gap-4">
        {sensorData.length === 0 ? (
          <p className="text-default-500 text-center py-4">No sensor data saved</p>
        ) : (
          <>
            {/* Site Information Section (BME Sensor + GPS) - Nested Card with Blue Accent */}
            <div className="rounded-lg border-2 border-primary/30 bg-primary/5 p-3">
              <div className="flex flex-row items-center justify-between mb-3">
                <h3 className="text-medium font-semibold text-primary">Site Information</h3>
                <div className="flex flex-row gap-4 flex-shrink-0">
                  <div className="flex flex-row gap-1 items-baseline whitespace-nowrap">
                    <span className="text-small text-default-500">Lat:</span>
                    <span className="text-lg font-semibold">{sensorLat}</span>
                  </div>
                  <div className="flex flex-row gap-1 items-baseline whitespace-nowrap">
                    <span className="text-small text-default-500">Long:</span>
                    <span className="text-lg font-semibold">{sensorLong}</span>
                  </div>
                </div>
              </div>
              <div className="grid grid-cols-3 gap-3">
                <div className="flex flex-col">
                  <span className="text-small text-default-500">Temperature (BME)</span>
                  <span className="text-lg font-semibold">{getSensorValue("BME Temperature")} °C</span>
                </div>
                <div className="flex flex-col">
                  <span className="text-small text-default-500">Pressure (BME)</span>
                  <span className="text-lg font-semibold">{getSensorValue("BME Pressure")} kPa</span>
                </div>
                <div className="flex flex-col">
                  <span className="text-small text-default-500">Altitude</span>
                  <span className="text-lg font-semibold">{getSensorValue("Altitude")} m</span>
                </div>
              </div>
            </div>

            {/* Soil Information Section (Hydraprobe) - Nested Card with Green Accent */}
            <div className="rounded-lg border-2 border-success/30 bg-success/5 p-3">
              <h3 className="text-medium font-semibold mb-3 text-success">Soil Information</h3>
              <div className="grid grid-cols-4 gap-3">
                <div className="flex flex-col">
                  <span className="text-small text-default-500">Temperature</span>
                  <span className="text-lg font-semibold">{getSensorValue("Temperature")} °C</span>
                </div>
                <div className="flex flex-col">
                  <span className="text-small text-default-500">Moisture</span>
                  <span className="text-lg font-semibold">{getSensorValue("Moisture")} %</span>
                </div>
                <div className="flex flex-col">
                  <span className="text-small text-default-500">Conductivity</span>
                  <span className="text-lg font-semibold">{getSensorValue("Conductivity")} mS/cm</span>
                </div>
                <div className="flex flex-col">
                  <span className="text-small text-default-500">Salinity</span>
                  <span className="text-lg font-semibold">{getSensorValue("Salinity")} ppt</span>
                </div>
              </div>
            </div>
          </>
        )}

        {/* NIR Results Section */}
        <NIRResultsWidget site={site} />
      </CardBody>
    </Card>
  );
};

export default SensorDataWidget;
