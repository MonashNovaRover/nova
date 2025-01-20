import {useGenericStore} from "../../hooks/useGenericStore.ts";
import {Site} from "../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteDataState} from "../../redux/models/genericStores/SiteDataState.ts";
import {useCallback} from "react";
import {ISpaceResourcesEntry} from "./SpaceResourcesSiteType.tsx";

export function useNIRSiteData(): [ISpaceResourcesEntry[], (a: ISpaceResourcesEntry[]) => void] {
  // current site as provided by the site selector
  const [currentSite, _] = useGenericStore<Site>("currentSite");

  // data related to each site
  const [siteData, setSiteData] = useGenericStore<SiteDataState>("siteData");
  const readings = siteData[currentSite].spaceResourcesEntries

  // function to update the current space resource entries
  const setReadings = useCallback((data: ISpaceResourcesEntry[]) => {
    setSiteData({
      ...siteData,
      [currentSite]: {
        ...siteData[currentSite],
        spaceResourcesEntries: data
      }
    } as SiteDataState)
  }, [currentSite, siteData, setSiteData]);

  return [readings, setReadings]
}
