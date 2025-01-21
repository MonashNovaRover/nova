import {
  CardProps, Table, TableBody,
  TableCell,
  TableColumn,
  TableHeader, TableRow
} from "@nextui-org/react";
import React from "react";
import {useCalibrationFunction} from "../NIRProbeCalibration/NIRCalibration.ts";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {XYNames} from "../SpaceResourcesSiteType.tsx";

export interface NIRProbeCalcTableProps extends CardProps {
}

const NIRProbeCalcTable: React.FC<NIRProbeCalcTableProps> = ({...cardProps}) => {

  const [readings, _] = useNIRSiteData()

  // calibration function
  const calibrationFunc = useCalibrationFunction()

  const xList = readings.map(entry => entry.x).filter(x => x)
  const averageX = xList.reduce((a,b) => a+b, 0) / Math.max(xList.length,1);

  const yList = readings.map(entry => entry.y).filter(y => y)
  const averageY = yList.reduce((a,b) => a+b, 0) / Math.max(yList.length,1);

  const tableRows = (
      <TableRow key="average">
        <TableCell key={1}>
          {averageX}
        </TableCell>
        <TableCell key={2}>
          {averageY}
        </TableCell>
        <TableCell key={3}>
          {averageY && averageX ? calibrationFunc(averageX, averageY) : "None"}
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