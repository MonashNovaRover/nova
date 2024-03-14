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
  setFile: (newFile: ISpaceResourcesFile) => void
}

const NIRProbeFileTableWidget: React.FC<NIRProbeFileTableWidgetProps> = ({
  file, setFile, ...cardProps
}) => {

  const deleteEntry = useCallback((index: number) => {
    const newFile = {
      entries: file.entries.filter((_, i) => i !== index)
    };

    setFile(newFile);
  }, [file]);

  // Get a reversed list of entries, so the most recent values can be displayed first
  const reversedFileEntries = [...file.entries];
  reversedFileEntries.reverse();

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe File Table
      </CardHeader>
      <CardBody className="flex flex-col gap-3">
        <Table aria-label="Example table with dynamic content">
          <TableHeader>
            <TableColumn key="lightBlank">Light Blank</TableColumn>
            <TableColumn key="difference">Difference</TableColumn>
            <TableColumn key="concentration">Concentration</TableColumn>
            <TableColumn key="label">Label</TableColumn>
            <TableColumn>Difference</TableColumn>
          </TableHeader>
          <TableBody>
            {reversedFileEntries.map(({lightBlank, difference, concentration, label}, index) =>
              <TableRow key={index}>
                <TableCell>
                  { lightBlank ?
                    <span className="text-gray-500">{lightBlank}</span> :
                    <span className="text-gray-800">{lightBlank ?? "None"}</span>
                  }
                </TableCell>
                <TableCell>{difference}</TableCell>
                <TableCell>
                  { concentration !== undefined ?
                    <span className="text-gray-500">{concentration}</span> :
                    <span className="text-gray-800">None</span>
                  }
                </TableCell>
                <TableCell>
                  {
                    label
                  }
                </TableCell>
                <TableCell>
                  <Button onClick={() => deleteEntry(reversedFileEntries.length - index - 1)}
                          size="sm" color="danger">
                    Delete
                  </Button>
                </TableCell>
              </TableRow>
            )}
          </TableBody>
        </Table>
      </CardBody>
    </Card>
  );
}


export default NIRProbeFileTableWidget;