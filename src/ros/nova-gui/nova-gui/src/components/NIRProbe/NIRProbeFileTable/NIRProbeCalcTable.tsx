import {CardProps, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow} from "@nextui-org/react";
import React from "react";
import {useAverageReading} from "../NIRProbeCalibration/NIRCalibration.ts";
import {
  XYNames
} from "../SpaceResourcesSiteType.tsx";

export interface NIRProbeCalcTableProps extends CardProps {
}

const NIRProbeCalcTable: React.FC<NIRProbeCalcTableProps> = () => {
  const [averageX, averageY, calibratedResult] = useAverageReading()

  const tableRows = (
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
      </TableRow>
    )

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