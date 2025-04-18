import {CardProps, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow} from "@nextui-org/react";
import React, {useMemo} from "react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {
  ISpaceResourcesEntry,
  NIRProbeReadingType,
  NIRProbeReadingTypeInfo,
} from "../SpaceResourcesSiteType.tsx";

export interface NIRProbeHeaderTableProps extends CardProps {
  readingInfo: NIRProbeReadingTypeInfo[] // list of NIRProbeReadingTypeInfo: [off, PD1, PD2]
}

const average = (data: ISpaceResourcesEntry[]) => {
  return (data.map(v => v.data).reduce((acc, v) => acc + v, 0) / data.length).toFixed(4)
}

/**
 * Table containing the average reading and concentration
 * @param readingInfo display information about each photodiode, should be of the form [off, PD1, PD2]
 * @constructor
 */
const NIRProbeSiteAvgTable: React.FC<NIRProbeHeaderTableProps> = ({readingInfo}) => {
  const [siteData, _] = useGenericStore<SiteDataState>("siteData");
  const PD1Icon = useMemo(() => readingInfo[1].icon, [readingInfo])
  const PD2Icon = useMemo(() => readingInfo[2].icon, [readingInfo])

  const tableRows = useMemo(() => [
    <TableRow key="average - PD1">
      <TableCell key={0}>
        <span className="flex flex-row gap-2">
          {<PD1Icon size={18}/>}
          {readingInfo[1].name}
        </span>
      </TableCell>
      <TableCell key={1}>{average(siteData[Site.SITE_1].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
      <TableCell key={2}>{average(siteData[Site.SITE_2].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
      <TableCell key={3}>{average(siteData[Site.SITE_3].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
      <TableCell key={4}>{average(siteData[Site.SITE_4].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
    </TableRow>,
    <TableRow key="average - PD2">
      <TableCell key={0}>
        <span className="flex flex-row gap-2">
          {<PD2Icon size={18}/>}
          {readingInfo[2].name}
        </span>
      </TableCell>
      <TableCell key={1}>{average(siteData[Site.SITE_1].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
      <TableCell key={2}>{average(siteData[Site.SITE_2].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
      <TableCell key={3}>{average(siteData[Site.SITE_3].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
      <TableCell key={4}>{average(siteData[Site.SITE_4].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
    </TableRow>,
  ], [PD1Icon, PD2Icon, readingInfo, siteData])

  return (
    <Table removeWrapper layout={"fixed"} className="table-fixed mb-3" aria-label="NIR probe readings table">
      <TableHeader>
        <TableColumn key="Site Type">Type</TableColumn>
        <TableColumn key="Site 1 AVG">Site 1 AVG</TableColumn>
        <TableColumn key="Site 2 AVG">Site 2 AVG</TableColumn>
        <TableColumn key="Site 3 AVG">Site 3 AVG</TableColumn>
        <TableColumn key="Site 4 AVG">Site 4 AVG</TableColumn>
      </TableHeader>
      <TableBody>
        {tableRows}
      </TableBody>
    </Table>
  );
}

export default NIRProbeSiteAvgTable;