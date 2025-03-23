import {useGenericStore} from "../../hooks/useGenericStore.ts";
import {Site} from "../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteDataState} from "../../redux/models/genericStores/SiteDataState.ts";
import {useCallback} from "react";
import {ISpaceResourcesEntries} from "./SpaceResourcesSiteType.tsx";

/**
 * Returns the saved NIR Probe readings at the current site
 */
export function useNIRSiteData(): [ISpaceResourcesEntries, (a: ((b: ISpaceResourcesEntries) => ISpaceResourcesEntries)) => void] {
  // current site as provided by the site selector
  const [currentSite, _] = useGenericStore<Site>("currentSite");

  // data related to each site
  const [siteData, setSiteData] = useGenericStore<SiteDataState>("siteData");
  const readings = siteData[currentSite].spaceResourcesEntries

  // function to update the current space resource entries
  // const setReadings = useCallback((data: ISpaceResourcesEntries) => {
  //   setSiteData({
  //     ...siteData,
  //     [currentSite]: {
  //       ...siteData[currentSite],
  //       spaceResourcesEntries: data
  //     }
  //   } as SiteDataState)
  // }, [currentSite, siteData, setSiteData]);

  // function to update the current space resource entries
  const setReadingsWithFunction = useCallback((reducer: ((b: ISpaceResourcesEntries) => ISpaceResourcesEntries)) => {
    setSiteData(oldSiteData => ({
      ...oldSiteData,
      [currentSite]: {
        ...siteData[currentSite],
        spaceResourcesEntries: reducer(oldSiteData[currentSite]?.spaceResourcesEntries ?? {})
      }
    }) as SiteDataState)
  }, [currentSite, siteData, setSiteData]);

  return [readings, setReadingsWithFunction]
}
