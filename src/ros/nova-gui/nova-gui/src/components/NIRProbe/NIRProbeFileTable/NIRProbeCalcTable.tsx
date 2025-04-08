import {CardProps, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow} from "@nextui-org/react";
import React from "react";
import {useAverageReading} from "../NIRProbeCalibration/NIRCalibration.ts";
import {NIRProbeReadingType, XYNames} from "../SpaceResourcesSiteType.tsx";
import {useNIRSiteData} from "../useNIRSiteData.ts";

export interface NIRProbeCalcTableProps extends CardProps {
}

/**
 * Table containing the average reading and concentration
 * @constructor
 */
const NIRProbeCalcTable: React.FC<NIRProbeCalcTableProps> = () => {
  const [averageX, averageY, calibratedResult] = useAverageReading()
  const [readings, _] = useNIRSiteData();

  const tableRows = [
      <TableRow key="average">
        <TableCell key={1}>
          {averageX.toFixed(4)}
        </TableCell>
        <TableCell key={2}>
          {averageY.toFixed(4)}
        </TableCell>
        <TableCell key={3}>
          {calibratedResult}
        </TableCell>
        <TableCell key={0}>{""}</TableCell>
      </TableRow>,
      <TableRow>
        <TableCell key={21}>
          {readings[NIRProbeReadingType.WATER].length}
        </TableCell>
        <TableCell key={22}>
          {readings[NIRProbeReadingType.ICE].length}
        </TableCell>
        <TableCell key={23}>{""}</TableCell>
        <TableCell key={20}>{""}</TableCell>
      </TableRow>
    ]

  return (
    <Table removeWrapper layout={"fixed"} className="table-fixed mb-3" aria-label="NIR probe readings table">
      <TableHeader>
        <TableColumn key="x">{XYNames.X} AVG</TableColumn>
        <TableColumn key="y">{XYNames.Y} AVG</TableColumn>
        <TableColumn key="fxy">{XYNames.FXY}</TableColumn>
        <TableColumn key="Action">STD</TableColumn>
      </TableHeader>
      <TableBody>
        {tableRows}
      </TableBody>
    </Table>
  );
}

export default NIRProbeCalcTable;