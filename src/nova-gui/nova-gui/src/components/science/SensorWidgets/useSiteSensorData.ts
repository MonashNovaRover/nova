import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {SensorData, SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";
import {useCallback} from "react";


/**
 * Hook to manage sensor data for the currently selected site.
 * Returns sensor data array, setter function, and add function to update sensors.
 */
export function useSiteSensorData(): [SensorData[], (data: SensorData[]) => void, (data: SensorData[]) => void] {
  // current site as provided by the site selector
  const [currentSite, _] = useGenericStore<Site>("currentSite");

  // data related to each site
  const [siteData, setSiteData] = useGenericStore<SiteDataState>("siteData");
  const sensorData = siteData[currentSite].sensorData

  // function to update the current sensor data entries
  const setSensorData = useCallback((data: SensorData[]) => {
    setSiteData({
      ...siteData,
      [currentSite]: {
        ...siteData[currentSite],
        sensorData: data
      }
    } as SiteDataState)
  }, [currentSite, siteData, setSiteData]);

  // adds sensor data, overriding any existing entries with the same names
  const addSensorData = useCallback((newData: SensorData[]) => {
    // Get the names of the new sensor data entries
    const newNames = new Set(newData.map(sensor => sensor.name));
    // Remove any existing sensor data with the same names and add the new ones
    const updatedSensorData = sensorData.filter(sensor => !newNames.has(sensor.name));
    updatedSensorData.push(...newData);
    setSensorData(updatedSensorData);
  }, [sensorData, setSensorData])

  return [sensorData, setSensorData, addSensorData]
}