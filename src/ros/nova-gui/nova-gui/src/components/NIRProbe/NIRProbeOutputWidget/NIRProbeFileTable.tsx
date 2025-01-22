import {
  Button, CardHeader,
  CardProps,
  Table,
  TableBody, TableCell,
  TableColumn,
  TableHeader, TableRow
} from "@nextui-org/react";
import React, {useCallback, useState} from "react";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {ISpaceResourcesEntry, XYNames} from "../SpaceResourcesSiteType.tsx";
import {Minimize2, Trash2, X} from "react-feather";
import {ToolTipButton} from "../../shared/TooltipButton.tsx";
import {toInteger} from "lodash";
import {useCalibrationFunction} from "../NIRProbeCalibration/NIRCalibration.ts";

const toInt = (val: string | number | undefined) => {
  if (val === undefined)
    return val

  return toInteger(val)
}

export interface NIRProbeFileTableProps extends CardProps {
  showAdvanced : boolean,
}

const NIRProbeFileTable: React.FC<NIRProbeFileTableProps> = ({
  showAdvanced
}) => {

  // NIR Probe readings data corresponding to the currently selected site.
  const [readings, setReadings] = useNIRSiteData()

  // currently selected readings index for use with merging rows
  const [selectedKeys, setSelectedKeys] = useState<Set<string | number>>(new Set([]));
  const [showMerge, setShowMerge] = useState<boolean>(false);

  // calibration function
  const calibrationFunc = useCalibrationFunction()

  const deleteEntry = useCallback((index: number) => {
    setReadings(readings.filter((_, i) => i !== index))
  }, [readings, setReadings]);

  const handleMergeClick = useCallback(() => {
    console.log("handling click")

    if (selectedKeys.size != 2) {
      console.error("Trying to merge NIR Probe readings but two rows have not been selected")
      return
    }

    const iter = selectedKeys.values()
    const first = toInt(iter.next().value)
    const second = toInt(iter.next().value)

    if (first === undefined || second === undefined)
      return

    const firstEntry = readings[first]
    const secEntry = readings[second]

    if ((firstEntry.x && secEntry.x) || (firstEntry.y && secEntry.y)) {
      console.error("trying to merge readings with duplicate x or y", firstEntry, secEntry)
      return
    }
    const x = firstEntry.x ? firstEntry.x : secEntry.x
    const y = firstEntry.y ? firstEntry.y : secEntry.y
    const label = firstEntry.label ? firstEntry.label : secEntry.label
    
    console.log(x, y, label)
    console.log(readings.map((v, i) => i === first
      ? {x: x, y: y, label: label} as ISpaceResourcesEntry
      : v
    ))

    setReadings(readings
      .map((v, i) => i === first
      ? {x: x, y: y, label: label} as ISpaceResourcesEntry
      : v
      ).filter((_, i) => i !== second)
    )
    setShowMerge(false);
    setSelectedKeys(new Set([]))
  }, [setShowMerge, setSelectedKeys, readings, setReadings, selectedKeys])


  const disabledKeys = () => {
    const set = readings.map((_, i) => i).filter(cantMerge).map((v) => `${v}`)
    return new Set(set)
  }

  /**
   * can only merge two readings if one has an x reading and one has a y readings
   * @param index
   */
  const cantMerge = (index: number) => {
    if (selectedKeys.has(index) || selectedKeys.has(`${index}`))
      return false

    if ((selectedKeys.size >= 2) || (readings[index].x && readings[index].y) )
      return true

    const key = toInt(selectedKeys.values().next().value)
    if (key === undefined)
      return false

    return ((readings[key].x && readings[index].x) || (readings[key].y && readings[index].y)) as boolean
 }

  const tableHeader = useCallback(() => {
    const cols = showAdvanced ? [XYNames.X, XYNames.Y, XYNames.FXY, "Label", "Action"]
      : [XYNames.X, XYNames.Y, XYNames.FXY, "Action"];

    return (
      <TableHeader>
        {cols.map((v) => <TableColumn>{v}</TableColumn>)}
      </TableHeader>
    )
  }, [showAdvanced])

  const entryRows = readings.map(({x, y, fxy, label}, index) => {
    const extras = showAdvanced ? [<TableCell key="label">{label}</TableCell>] : [];
    if (!fxy && x && y)
      setReadings(readings.map((v, i) => i === index ? {...v, fxy: calibrationFunc(v.x, v.y)} : v));

    const cells = [
      <TableCell key={"x-"+index}>{x ? x : "-"}</TableCell>,
      <TableCell key={"y-"+index}>{y ? y : "-"}</TableCell>,
      <TableCell key={"fxy-"+index}>{fxy ? fxy.toFixed(4) : "-"}</TableCell>,
      ...extras,
      <TableCell key="action">
        <Button onPress={() => deleteEntry(index)}
                size="sm" color="danger" variant="light" className="w-full">
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
        {!showMerge && <ToolTipButton
          tooltipContent="Merge readings together"
          variant="light"
          size="sm"
          onClick={() => setShowMerge(true)}
        >
          <Minimize2/>
        </ToolTipButton>}
        {showMerge && <Button size="sm" variant="light" onClick={handleMergeClick}
        disabled={selectedKeys.size != 2}>
            Merge
        </Button>}
        {showMerge && <Button size="sm" variant="light" onClick={() => {
          setShowMerge(false);
          setSelectedKeys(new Set([]))
        }}>
            <X/>
        </Button>}
      </CardHeader>
      <Table
        removeWrapper
        layout={"fixed"}
        selectedKeys={selectedKeys}
        disabledKeys={showMerge ? disabledKeys() : []}
        selectionMode={showMerge ? "multiple" : "none"}
        onSelectionChange={(s) => s === "all" ? setSelectedKeys(new Set([0, 1])) : setSelectedKeys(s)}
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