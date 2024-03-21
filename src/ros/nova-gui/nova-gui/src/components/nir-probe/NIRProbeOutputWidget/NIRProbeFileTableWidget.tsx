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
import React, {useCallback} from "react";

import {ISpaceResourcesFile} from "./NIRProbeWidget.tsx";


export interface NIRProbeFileTableWidgetProps extends CardProps {
  file: ISpaceResourcesFile,
  setFile: (newFile: ISpaceResourcesFile) => void,
  showAdvanced : boolean,
  absorbance: (v: number) => number,
  calibrationFunction: (v: number) => number,
}

const NIRProbeFileTableWidget: React.FC<NIRProbeFileTableWidgetProps> = ({
  file, setFile, showAdvanced, absorbance, calibrationFunction, ...cardProps
}) => {

  const deleteEntry = useCallback((index: number) => {
    const newFile = {
      ...file,
      entries: file.entries.filter((_, i) => i !== index)
    };

    setFile(newFile);
  }, [file]);

  // Get a reversed list of entries, so the most recent values can be displayed first
  const reversedFileEntries = [...file.entries];
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
        <TableCell>
          { (lightBlank ?
            <span className="text-gray-500">{lightBlank}</span> :
            <span className="text-gray-800">{lightBlank ?? "None"}</span>)
          }
        </TableCell>
        <TableCell>{difference}</TableCell>
        <TableCell>
          { showAdvanced && (concentration !== undefined ?
            <span className="text-gray-500">{concentration}</span> :
            <span className="text-gray-800">None</span>)
          }
        </TableCell>
        <TableCell>{label}</TableCell>
        <TableCell>
          <Button onClick={() => deleteEntry(reversedFileEntries.length - index - 1)}
                  size="sm" color="danger" variant="light" className="block w-full">
            Delete
          </Button>
        </TableCell>
      </TableRow>
      :
      <TableRow key={index}>
        <TableCell>{difference}</TableCell>
        <TableCell>{calibrationFunction(absorbance(difference)).toFixed(4)}</TableCell>
        <TableCell>{absorbance(difference).toFixed(4)}</TableCell>
        <TableCell>
          <Button onClick={() => deleteEntry(reversedFileEntries.length - index - 1)}
                  size="sm" color="danger" variant="light" className="block w-full">
            Delete
          </Button>
        </TableCell>
      </TableRow>
  );

  const headerCell = <TableCell className="absolute text-small uppercase tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400">Site Average</TableCell>;
  const headerRow = (
    showAdvanced ?
      <TableRow className="relative h-6">
        {headerCell}
        <TableCell>{""}</TableCell>
        <TableCell>{""}</TableCell>
        <TableCell>{""}</TableCell>
        <TableCell>{""}</TableCell>
      </TableRow>
      :
      <TableRow className="relative h-6">
        {headerCell}
        <TableCell>{""}</TableCell>
        <TableCell>{""}</TableCell>
        <TableCell>{""}</TableCell>
      </TableRow>
  )

  const averageDifference = file.entries.map(entry => entry.difference).reduce((a,b) => a+b, 0) / Math.max(file.entries.length,1);
  const averageRow = (
    showAdvanced ?
      <TableRow key="average">
        <TableCell>{""}</TableCell>
        <TableCell>
          {averageDifference}
        </TableCell>
        <TableCell>
          {absorbance?.(averageDifference)}
        </TableCell>
        <TableCell>{""}</TableCell>
        <TableCell>{""}</TableCell>
      </TableRow>
      :
      <TableRow key="average">
        <TableCell>
          {averageDifference}
        </TableCell>
        <TableCell>{calibrationFunction(absorbance(averageDifference)).toFixed(4)}</TableCell>
        <TableCell>{absorbance(averageDifference).toFixed(4)}</TableCell>
        <TableCell>
          {""}
        </TableCell>
      </TableRow>
  )

  if (file.entries.length > 0) {
    entryRows.push(headerRow)
    entryRows.push(averageRow);
  }

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe File Table
      </CardHeader>
      <CardBody className="flex flex-col gap-3 p-0">
        <Table aria-label="NIR probe readings table">
          {tableHeader}
          <TableBody>
            {entryRows}
          </TableBody>
        </Table>
      </CardBody>
    </Card>
  );
}


export default NIRProbeFileTableWidget;