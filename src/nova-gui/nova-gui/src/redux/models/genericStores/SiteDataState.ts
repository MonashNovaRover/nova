import {Site} from "./CurrentSiteStore.ts";
import {
  ISpaceResourcesEntries,
  SpaceResourcesSiteType
} from "../../../components/science/NIRProbe/SpaceResourcesSiteType.tsx";
import {ThresholdingFileEntry} from "../../../components/science/MicroscopeThresholdWidget/MicroscopeThresholdWidget.tsx";

// Simple data structure to hold arbitrary sensor data.
export interface SensorData {
  name: string,
  data: number,
}

// Data that is associated with a site can be stored here.
export interface SiteData {
  siteType: SpaceResourcesSiteType,
  spaceResourcesEntries: ISpaceResourcesEntries,
  thresholdingEntries: ThresholdingFileEntry[],
  sensorData: SensorData[],
}

export interface SiteDataState {
  [Site.SITE_1]: SiteData,
  [Site.SITE_2]: SiteData,
  [Site.SITE_3]: SiteData,
  [Site.SITE_4]: SiteData,
}

// Default site data value
const EMPTY_SITE_DATA: SiteData = {
  siteType: SpaceResourcesSiteType.WATER,
  spaceResourcesEntries: {1: [], 2: []},
  thresholdingEntries: [],
  sensorData: [],
};

export const initialSiteDataState: SiteDataState = {
  [Site.SITE_1]: EMPTY_SITE_DATA,
  [Site.SITE_2]: EMPTY_SITE_DATA,
  [Site.SITE_3]: EMPTY_SITE_DATA,
  [Site.SITE_4]: EMPTY_SITE_DATA,
};
