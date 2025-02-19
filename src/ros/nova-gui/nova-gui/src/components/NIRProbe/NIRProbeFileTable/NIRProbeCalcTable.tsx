import {CardProps, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow} from "@nextui-org/react";
import React, {useMemo} from "react";
import {useCalibrationFunction} from "../NIRProbeCalibration/NIRCalibration.ts";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {
  NIRProbeReadingType,
  XYNames
} from "../SpaceResourcesSiteType.tsx";

export interface NIRProbeCalcTableProps extends CardProps {
}

const NIRProbeCalcTable: React.FC<NIRProbeCalcTableProps> = () => {

  const [readings, _] = useNIRSiteData()

  // calibration function
  const calibrationFunc = useCalibrationFunction()

  // average water reading
  const xList = useMemo(() => readings[NIRProbeReadingType.WATER]
    .map(entry => entry.data),
    [readings])
  const averageX = useMemo(() =>
    xList.reduce((a,b) => a+b, 0) / Math.max(xList.length,1),
    [xList]
  )

  // average ice reading
  const yList = useMemo(() => readings[NIRProbeReadingType.ICE]
      .map(entry => entry.data),
    [readings])
  const averageY = useMemo(() =>
    yList.reduce((a,b) => a+b, 0) / Math.max(yList.length,1),
    [yList]
  )

  const tableRows = (
      <TableRow key="average">
        <TableCell key={1}>
          {averageX.toFixed(4)}
        </TableCell>
        <TableCell key={2}>
          {averageY.toFixed(4)}
        </TableCell>
        <TableCell key={3}>
          {averageY && averageX ? calibrationFunc(averageX, averageY).toFixed(4) : "None"}
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