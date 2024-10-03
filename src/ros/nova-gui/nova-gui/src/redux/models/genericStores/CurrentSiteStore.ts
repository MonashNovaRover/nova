import SpaceResourceSiteType from "../../../components/nir-probe/SpaceResourcesSiteType.tsx";

export interface CurrentSiteStore {
  site: Site,
  type: SpaceResourceSiteType,
}

export enum Site {
  SITE_1,
  SITE_2,
  SITE_3,
  SITE_4,
}
