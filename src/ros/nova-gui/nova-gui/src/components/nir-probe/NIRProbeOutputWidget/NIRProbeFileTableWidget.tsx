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
}

const NIRProbeFileTableWidget: React.FC<NIRProbeFileTableWidgetProps> = ({
  file, setFile, showAdvanced, ...cardProps
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

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe File Table
      </CardHeader>
      <CardBody className="flex flex-col gap-3 p-0">
        <Table aria-label="Example table with dynamic content">
          { showAdvanced ? <TableHeader>
            <TableColumn key="lightBlank">Light Blank</TableColumn>
            <TableColumn key="difference">Difference</TableColumn>
            <TableColumn key="concentration">Concentration</TableColumn>
            <TableColumn key="label">Label</TableColumn>
            <TableColumn>Action</TableColumn>
          </TableHeader> : <TableHeader>
            <TableColumn key="difference">Difference</TableColumn>
            <TableColumn>Action</TableColumn>
          </TableHeader>}
          <TableBody>
            {reversedFileEntries.map(({lightBlank, difference, concentration, label}, index) =>
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
                          size="sm" color="danger">
                    Delete
                  </Button>
                </TableCell>
              </TableRow>
              :
              <TableRow key={index}>
                <TableCell>{difference}</TableCell>
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