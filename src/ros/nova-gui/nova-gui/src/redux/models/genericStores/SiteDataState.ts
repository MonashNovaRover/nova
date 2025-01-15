import {Site} from "./CurrentSiteStore.ts";
import {ISpaceResourcesEntry, SpaceResourcesSiteType} from "../../../components/NIRProbe/SpaceResourcesSiteType.tsx";
import {ThresholdingFileEntry} from "../../../components/MicroscopeThresholdWidget/MicroscopeThresholdWidget.tsx";

// Data that is associated with a site can be stored here.
export interface SiteData {
  siteType: SpaceResourcesSiteType,
  spaceResourcesEntries: ISpaceResourcesEntry[],
  thresholdingEntries: ThresholdingFileEntry[]
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
  spaceResourcesEntries: [],
  thresholdingEntries: []
};

export const initialSiteDataState: SiteDataState = {
  [Site.SITE_1]: EMPTY_SITE_DATA,
  [Site.SITE_2]: EMPTY_SITE_DATA,
  [Site.SITE_3]: EMPTY_SITE_DATA,
  [Site.SITE_4]: EMPTY_SITE_DATA,
};
