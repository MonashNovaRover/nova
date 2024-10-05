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
import {SiteData} from "../../../redux/models/genericStores/SiteDataState.ts";

export interface NIRProbeFileTableWidgetProps extends CardProps {
  file: SiteData,
  setFile: (newFile: SiteData) => void,
  showAdvanced : boolean,
  absorbance: (v: number) => number,
  calibrationFunction: (v: number) => number,
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
  file, setFile, showAdvanced, absorbance, calibrationFunction, ...cardProps
}) => {

  const deleteEntry = useCallback((index: number) => {
    const newFile: SiteData = {
      ...file,
      spaceResourcesEntries: file.spaceResourcesEntries.filter((_, i) => i !== index)
    };

    setFile(newFile);
  }, [file, setFile]);

  // Get a reversed list of entries, so the most recent values can be displayed first
  const reversedFileEntries = [...file.spaceResourcesEntries];
  reversedFileEntries.reverse();

  const tableHeader = (
    showAdvanced ?
      <TableHeader>
        <TableColumn key="lightBlank">Light Blank</TableColumn>
        <TableColumn key="difference">Difference</TableColumn>
        <TableColumn key="concentration">Concentration</TableColumn>
        <TableColumn key="label">Label</TableColumn>
        <TableColumn>Action</TableColumn>
      </TableHeader>
      :
      <TableHeader>
        <TableColumn key="difference">Difference</TableColumn>
        <TableColumn key="concentration">Concentration</TableColumn>
        <TableColumn key="absorbance">Absorbance</TableColumn>
        <TableColumn>Action</TableColumn>
      </TableHeader>
  )

  const entryRows = reversedFileEntries.map(({lightBlank, difference, concentration, label}, index) =>
    showAdvanced ?
      <TableRow key={index}>
        <TableCell key="lightBlank">
          { (lightBlank ?
            <span className="text-gray-500">{lightBlank}</span> :
            <span className="text-gray-800">{lightBlank ?? "None"}</span>)
          }
        </TableCell>
        <TableCell key="difference">{difference}</TableCell>
        <TableCell key="concentration">
          { showAdvanced && (concentration !== undefined ?
            <span className="text-gray-500">{concentration}</span> :
            <span className="text-gray-800">None</span>)
          }
        </TableCell>

        <TableCell key="label">{label}</TableCell>
        <TableCell key="action">
          <Button onPress={() => deleteEntry(reversedFileEntries.length - index - 1)}
                  size="sm" color="danger" variant="light" className="block w-full">
            Delete
          </Button>
        </TableCell>
      </TableRow>
      :
      <TableRow key={index}>
        <TableCell key="difference">{difference}</TableCell>
        <TableCell key="concentration">{calibrationFunction(absorbance(difference)).toFixed(4)}</TableCell>
        <TableCell key="absorbance">{absorbance(difference).toFixed(4)}</TableCell>
        <TableCell key="action">
          <Button onPress={() => deleteEntry(reversedFileEntries.length - index - 1)}
                  size="sm" color="danger" variant="light" className="block w-full">
            Delete
          </Button>
        </TableCell>
      </TableRow>
  );

  const averageHeaderCell = <TableCell className="absolute text-small uppercase tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400" key={0}>Site Average</TableCell>;
  const averageHeaderRow = RowFromHeader(averageHeaderCell, showAdvanced ? 5 : 4, "averageHeader");

  const readingHeaderCell = <TableCell className="absolute text-small uppercase tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400" key={0}>Site Readings</TableCell>;
  const readingHeaderRow = RowFromHeader(readingHeaderCell, showAdvanced ? 5 : 4, "reading");

  const noReadingHeaderCell = <TableCell className="absolute text-small tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400" key={0}>No readings recorded</TableCell>;
  const noReadingHeaderRow = RowFromHeader(noReadingHeaderCell, showAdvanced ? 5 : 4, "noReading");

  const averageDifference = file.spaceResourcesEntries.map(entry => entry.difference).reduce((a,b) => a+b, 0) / Math.max(file.spaceResourcesEntries.length,1);
  const averageRow = (
    showAdvanced ?
      <TableRow key="average">
        <TableCell key={0}>{""}</TableCell>
        <TableCell key={1}>
          {averageDifference}
        </TableCell>
        <TableCell key={2}>
          {absorbance?.(averageDifference)}
        </TableCell>
        <TableCell key={3}>{""}</TableCell>
        <TableCell key={4}>{""}</TableCell>
      </TableRow>
      :
      <TableRow key="average">
        <TableCell key={1}>
          {averageDifference}
        </TableCell>
        <TableCell key={2}>{calibrationFunction(absorbance(averageDifference)).toFixed(4)}</TableCell>
        <TableCell key={3}>{absorbance(averageDifference).toFixed(4)}</TableCell>
        <TableCell key={4}>{""}</TableCell>
      </TableRow>
  )

  const tableRows = file.spaceResourcesEntries.length > 0 ?
    [averageHeaderRow, averageRow, readingHeaderRow, ...entryRows] :
    [noReadingHeaderRow];

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe File Table
      </CardHeader>
      <CardBody className="flex flex-col gap-3 p-0">
        <Table aria-label="NIR probe readings table">
          {tableHeader}
          <TableBody>
            {tableRows}
          </TableBody>
        </Table>
      </CardBody>
    </Card>
  );
}


export default NIRProbeFileTableWidget;