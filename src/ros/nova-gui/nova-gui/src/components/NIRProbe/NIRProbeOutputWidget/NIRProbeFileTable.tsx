import {
  Button, CardHeader,
  CardProps, Selection,
  Table,
  TableBody, TableCell,
  TableColumn,
  TableHeader, TableRow
} from "@nextui-org/react";
import React, {ReactElement, useCallback, useState} from "react";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {XYNames} from "../SpaceResourcesSiteType.tsx";
import {Minimize2, Trash2} from "react-feather";
import {ArrowsCollapse} from "react-bootstrap-icons";
import {ToolTipButton} from "../../shared/TooltipButton.tsx";

export interface NIRProbeFileTableProps extends CardProps {
  showAdvanced : boolean,
}

const NIRProbeFileTable: React.FC<NIRProbeFileTableProps> = ({
  showAdvanced, ...cardProps
}) => {

  // NIR Probe readings data corresponding to the currently selected site.
  const [readings, setReadings] = useNIRSiteData()

  // currently selected readings index for use with merging rows
  const [selectedKeys, setSelectedKeys] = useState<Selection>(new Set([]));

  // calibration function
  // const calibrationFunc = useCalibrationFunction()

  const deleteEntry = useCallback((index: number) => {
    setReadings(readings.filter((_, i) => i !== index))
  }, [readings, setReadings]);

  // Get a reversed list of entries, so the most recent values can be displayed first
  const reversedFileEntries = [...readings];
  reversedFileEntries.reverse();
  //
  // const handleMergeClick = useCallback((index: number) => {
  //   console.log("handling click")
  //   if (selected === undefined) {
  //     setSelected(index)
  //     return
  //   }
  //   if (selected === index) {
  //     setSelected(undefined)
  //     return
  //   }
  //   // we now have two indexes attempting to merge.
  //
  //   const firstEntry = readings[selected]
  //   const secEntry = readings[index]
  //   if ((firstEntry.x && secEntry.x) || (firstEntry.y && secEntry.y)) {
  //     console.error("trying to merge readings with duplicate x or y", firstEntry, secEntry)
  //     return
  //   }
  //   const x = firstEntry.x ? firstEntry.x : secEntry.x
  //   const y = firstEntry.y ? firstEntry.y : secEntry.y
  //   const label = firstEntry.label ? firstEntry.label : secEntry.label
  //
  //   setReadings(readings.map((v, i) => i === selected
  //     ? {x: x, y: y, label: label}
  //     : v
  //   ))
  //   deleteEntry(index)
  //   setSelected(undefined)
  // }, [selected, setSelected, deleteEntry])
  //
  // /**
  //  * can only merge two readings if one has an x reading and one has a y readings
  //  * @param index
  //  */
  // const canMerge = (index: number) => {
  //   return ((selected !== undefined) && (selected !== index) && ((readings[selected].x && readings[index].x) || (readings[selected].y && readings[index].y))) as boolean
  // }

  const tableHeader = useCallback(() => {
    const cols = showAdvanced ? [XYNames.X, XYNames.Y, XYNames.FXY, "Label", "Action"]
      : [XYNames.X, XYNames.Y, XYNames.FXY, "Action"];

    return (
      <TableHeader>
        {cols.map((v) => <TableColumn>{v}</TableColumn>)}
      </TableHeader>
    )
  }, [showAdvanced])

  const entryRows = reversedFileEntries.map(({x, y, fxy, label}, index) => {
    const extras = showAdvanced ? [<TableCell key="label">{label}</TableCell>] : [];
    // const mergeDisabled = canMerge(index)

    const cells = [
      <TableCell key={"x-"+index}>{x ? x : "None"}</TableCell>,
      <TableCell key={"y-"+index}>{y ? y : "None"}</TableCell>,
      <TableCell key={"fxy-"+index}>{fxy ? fxy.toFixed(4) : "None"}</TableCell>,
      ...extras,
      <TableCell key="action">
        <Button onPress={() => deleteEntry(reversedFileEntries.length - index - 1)}
                size="sm" color="danger" variant="light" className="">
          <Trash2/>
        </Button>
      </TableCell>,
    ];

    return (<TableRow key={index}>
      {cells}
    </TableRow>);
  });

  return (
    <div>
      <CardHeader className="pl-0 flex flex-row gap-3">
        <div className="grow">NIR Probe Readings</div>
        <ToolTipButton tooltipContent="Merge readings together" variant="light">
          <Minimize2/>
        </ToolTipButton>
      </CardHeader>
      <Table
        removeWrapper
        layout={"fixed"}
        selectedKeys={selectedKeys}
        selectionMode={"multiple"}
        onSelectionChange={setSelectedKeys}
        className=""
        aria-label="NIR probe readings table"
      >
        {tableHeader()}
        <TableBody emptyContent={"No readings recorded."}>
          {entryRows}
        </TableBody>
      </Table>
    </div>
  );
}

export default NIRProbeFileTable;