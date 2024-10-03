import SpaceResourceSiteType from "../../../components/nir-probe/SpaceResourcesSiteType.tsx";
import {Site} from "./CurrentSiteStore.ts";

interface ISpaceResourcesEntry {
  lightBlank?: number,
  difference: number,
  concentration?: number,
  label: string
}

export interface ISpaceResourcesFile {
  entries: ISpaceResourcesEntry[],
  type: SpaceResourceSiteType
}

export interface NIRProbeFilesState {
  [Site.SITE_1]: ISpaceResourcesFile,
  [Site.SITE_2]: ISpaceResourcesFile,
  [Site.SITE_3]: ISpaceResourcesFile,
  [Site.SITE_4]: ISpaceResourcesFile,
}

const EMPTY_SPACE_RESOURCES_FILE: ISpaceResourcesFile = {
  entries: [],
  type: SpaceResourceSiteType.WATER
};

export const initialNIRProbeFilesState = {
  [Site.SITE_1]: EMPTY_SPACE_RESOURCES_FILE,
  [Site.SITE_2]: EMPTY_SPACE_RESOURCES_FILE,
  [Site.SITE_3]: EMPTY_SPACE_RESOURCES_FILE,
  [Site.SITE_4]: EMPTY_SPACE_RESOURCES_FILE,
} as NIRProbeFilesState;