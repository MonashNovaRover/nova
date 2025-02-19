import {
  Button,
  CardProps, Input,
  Tab,
  Table,
  TableBody,
  TableCell,
  TableColumn,
  TableHeader,
  TableRow,
  Tabs
} from "@nextui-org/react";
import React, {useCallback, useMemo} from "react";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {ISpaceResourcesEntries, NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";
import {Droplet, Square, Trash2} from "react-feather";
import {useAbsorbance} from "../NIRProbeCalibration/NIRCalibration.ts";

export interface NIRProbeFileTableProps extends CardProps {
}

const NIRProbeFileTable: React.FC<NIRProbeFileTableProps> = () => {

  // NIR Probe readings data corresponding to the currently selected site.
  const [readings, setReadings] = useNIRSiteData();
  const absorbance = useAbsorbance();

  const deleteEntry = useCallback((index: number, type: keyof ISpaceResourcesEntries) => {
    setReadings({
      ...readings,
      [type]: readings[type].filter((_, i) => i !== index)
    })
  }, [readings, setReadings]);

  const setLabel = useCallback((index: number, type: keyof ISpaceResourcesEntries) => (label: string) => {
    setReadings({
      ...readings,
      [type]: readings[type].map((v, i) => i !== index ? v : {
        ...v,
        label: label,
      }),
    })
  }, [readings, setReadings])

  const tableHeader = useCallback(() => (
    <TableHeader>
      {["Reading (Difference)", "Absorbance", "Label", "Action"]
        .map((v) => <TableColumn key={`header-${v}`}>{v}</TableColumn>)}
    </TableHeader>
  ), [])

  const entryRows = useCallback((type: NIRProbeReadingType.WATER | NIRProbeReadingType.ICE) =>
    readings[type]
      .map(({data, type, label}, index) => (
        <TableRow key={index}>
          <TableCell key={"reading-"+index}>{data}</TableCell>
          <TableCell key={"absorbance-"+index}>{absorbance(type, data).toFixed(4)}</TableCell>
          <TableCell key="label">
            <Input size="sm" value={label} onValueChange={setLabel(index, type as keyof ISpaceResourcesEntries)}/>
          </TableCell>
          <TableCell key="action">
            <Button onPress={() => deleteEntry(index, type as keyof ISpaceResourcesEntries)}
                    size="sm" color="danger" variant="light" className="w-full" key="delete-button">
              <Trash2/>
            </Button>
          </TableCell>
        </TableRow>
    )), [readings, deleteEntry, absorbance, setLabel])

  const table = useCallback((type: NIRProbeReadingType.WATER | NIRProbeReadingType.ICE) => (
    <Table
      removeWrapper
      layout={"fixed"}
      aria-label="NIR probe readings table"
    >
      {tableHeader()}
      <TableBody emptyContent={"No readings recorded."}>
        {entryRows(type)}
      </TableBody>
    </Table>
  ), [entryRows, tableHeader])

  const waterTable = useMemo(() => table(NIRProbeReadingType.WATER), [table])
  const iceTable = useMemo(() => table(NIRProbeReadingType.ICE), [table])

  return (
      <div className="flex w-full flex-col">
        <Tabs
          aria-label="NIR-Probe-Options"
          classNames={{
            tabList: "gap-6 w-full relative rounded-none p-0 border-b border-divider",
            cursor: "w-full bg-[#22d3ee]",
            tab: "max-w-fit px-0 h-12",
            tabContent: "group-data-[selected=true]:text-[#06b6d4]",
          }}
          color="primary"
          variant="underlined"
        >
          <Tab
            key={`${NIRProbeReadingType.WATER}`}
            title={
              <div className="flex items-center space-x-2">
                <Droplet />
                <span>Water</span>
              </div>
            }
          >
            {waterTable}
          </Tab>
          <Tab
            key={`${NIRProbeReadingType.ICE}`}
            title={
              <div className="flex items-center space-x-2">
                <Square/>
                <span>Ice</span>
              </div>
            }
          >
            {iceTable}
          </Tab>
        </Tabs>
      </div>
  );
}

export default NIRProbeFileTable;