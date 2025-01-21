import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
  TableCell,
  TableColumn,
  TableHeader, TableRow
} from "@nextui-org/react";
import React, {ReactElement, useCallback} from "react";
import {useCalibrationFunction} from "../NIRProbeCalibration/NIRCalibration.ts";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import NIRProbeFileTable from "./NIRProbeFileTable.tsx";
import NIRProbeCalcTable from "./NIRProbeCalcTable.tsx";

export interface NIRProbeFileTableWidgetProps extends CardProps {
  showAdvanced : boolean,
}

const RowFromHeader = (tableHeader: ReactElement, numRows: number, key?: string) => {
  return (
    <TableRow className="relative h-6" key={key}>
      {[
        tableHeader,
        ...Array.from({length: numRows-1}, (_, i) => (
          <TableCell key={i+1}>{""}</TableCell>
        ))
      ]}
    </TableRow>
  )
}

const NIRProbeFileTableWidget: React.FC<NIRProbeFileTableWidgetProps> = ({
  showAdvanced, ...cardProps
}) => {

  const [readings, setReadings] = useNIRSiteData()

  // calibration function
  const calibrationFunc = useCalibrationFunction()

  const tableHeader = useCallback(() => {
    const extras = showAdvanced ? [<TableColumn key="label">Label</TableColumn>] : [];

    const columns = [
      <TableColumn key="x">X</TableColumn>,
      <TableColumn key="y">Y</TableColumn>,
      <TableColumn key="fxy">f(x,y)</TableColumn>,
      ...extras,
      <TableColumn key="Action">Action</TableColumn>,
    ]
    return (
      <TableHeader>
        {columns}
      </TableHeader>
    )
  }, [showAdvanced])





  const xList = readings.map(entry => entry.x).filter(x => x)
  const averageX = xList.reduce((a,b) => a+b, 0) / Math.max(xList.length,1);

  const yList = readings.map(entry => entry.y).filter(y => y)
  const averageY = yList.reduce((a,b) => a+b, 0) / Math.max(yList.length,1);

  const averageRow =
    showAdvanced ? (
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
        <TableCell>{""}</TableCell>
        <TableCell>{""}</TableCell>
      </TableRow>
    ) : (
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
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe File Table
      </CardHeader>
      <CardBody className="flex flex-col gap-3 p-3">
        <NIRProbeCalcTable/>
        <NIRProbeFileTable showAdvanced={showAdvanced}/>
      </CardBody>
    </Card>
  );
}

export default NIRProbeFileTableWidget;