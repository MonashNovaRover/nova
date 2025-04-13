import {CardProps, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow} from "@nextui-org/react";
import React, {useMemo} from "react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {ISpaceResourcesEntry, NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";

export interface NIRProbeCalcTableProps extends CardProps {
}

const average = (data: ISpaceResourcesEntry[]) => {
  return (data.map(v => v.data).reduce((acc, v) => acc + v, 0) / data.length).toFixed(4)
}

/**
 * Table containing the average reading and concentration
 * @constructor
 */
const NIRProbeSiteAvgTable: React.FC<NIRProbeCalcTableProps> = () => {
  const [siteData, _] = useGenericStore<SiteDataState>("siteData");

  const tableRows = useMemo(() => [
    <TableRow key="average - PD1">
      <TableCell key={0}>PD1</TableCell>
      <TableCell key={1}>{average(siteData[Site.SITE_1].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
      <TableCell key={2}>{average(siteData[Site.SITE_2].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
      <TableCell key={3}>{average(siteData[Site.SITE_3].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
      <TableCell key={4}>{average(siteData[Site.SITE_4].spaceResourcesEntries[NIRProbeReadingType.PD1])}</TableCell>
    </TableRow>,
    <TableRow key="average - PD2">
      <TableCell key={0}>PD2</TableCell>
      <TableCell key={1}>{average(siteData[Site.SITE_1].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
      <TableCell key={2}>{average(siteData[Site.SITE_2].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
      <TableCell key={3}>{average(siteData[Site.SITE_3].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
      <TableCell key={4}>{average(siteData[Site.SITE_4].spaceResourcesEntries[NIRProbeReadingType.PD2])}</TableCell>
    </TableRow>,
  ], [siteData])

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