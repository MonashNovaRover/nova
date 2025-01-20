import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
  Table,
  TableBody, TableCell,
  TableColumn,
  TableHeader, TableRow
} from "@nextui-org/react";
import React, {ReactElement, useCallback} from "react";
import {useCalibrationFunction} from "../NIRProbeCalibration/NIRCalibration.ts";
import {useNIRSiteData} from "../useNIRSiteData.ts";

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

  const deleteEntry = useCallback((index: number) => {
    setReadings(readings.filter((_, i) => i !== index))
  }, [readings, setReadings]);

  // Get a reversed list of entries, so the most recent values can be displayed first
  const reversedFileEntries = [...readings];
  reversedFileEntries.reverse();

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

  const entryRows = reversedFileEntries.map(({x, y, fxy, label}, index) => {
    const extras = showAdvanced ? [<TableCell key="label">{label}</TableCell>] : [];

    const cells = [
      <TableCell key={"x-"+index}>{x ? x : "None"}</TableCell>,
      <TableCell key={"y-"+index}>{y ? y : "None"}</TableCell>,
      <TableCell key={"fxy-"+index}>{fxy ? fxy.toFixed(4) : "None"}</TableCell>,
      ...extras,
      <TableCell key="action">
        <Button onPress={() => deleteEntry(reversedFileEntries.length - index - 1)}
                size="sm" color="danger" variant="light" className="block w-full">
          Delete
        </Button>
      </TableCell>,
    ];

    return (<TableRow key={"NIRProbeEntry-"+index}>
      {cells}
    </TableRow>);
  });

  const averageHeaderCell = <TableCell className="absolute text-small uppercase tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400" key={0}>Site Average</TableCell>;
  const averageHeaderRow = RowFromHeader(averageHeaderCell, showAdvanced ? 5 : 4, "averageHeader");

  const readingHeaderCell = <TableCell className="absolute text-small uppercase tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400" key={0}>Site Readings</TableCell>;
  const readingHeaderRow = RowFromHeader(readingHeaderCell, showAdvanced ? 5 : 4, "reading");

  const noReadingHeaderCell = <TableCell className="absolute text-small tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400" key={0}>No readings recorded</TableCell>;
  const noReadingHeaderRow = RowFromHeader(noReadingHeaderCell, showAdvanced ? 5 : 4, "noReading");

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


  const tableRows = readings.length > 0 ?
    [averageHeaderRow, averageRow, readingHeaderRow, ...entryRows] :
    [noReadingHeaderRow];

  console.log(tableRows)
  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe File Table
      </CardHeader>
      <CardBody className="flex flex-col gap-3 p-0">
        <Table className="table-fixed" aria-label="NIR probe readings table">
          {tableHeader()}
          <TableBody>
            {tableRows}
          </TableBody>
        </Table>
      </CardBody>
    </Card>
  );
}

export default NIRProbeFileTableWidget;