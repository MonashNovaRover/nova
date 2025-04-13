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
import {ISpaceResourcesEntries, NIRProbeReadingType, NIRProbeReadingTypeInfo} from "../SpaceResourcesSiteType.tsx";
import {Trash2} from "react-feather";

export interface NIRProbeFileTableProps extends CardProps {
  readingInfo: NIRProbeReadingTypeInfo[]
}

/**
 * Table containing all recorded NIR Probe readings
 * @constructor
 */
const NIRProbeFileTable: React.FC<NIRProbeFileTableProps> = ({readingInfo}: NIRProbeFileTableProps) => {

  // NIR Probe readings data corresponding to the currently selected site.
  const [readings, setReadings] = useNIRSiteData();
  const PD1Icon = useMemo(() => readingInfo[1].icon, [readingInfo])
  const PD2Icon = useMemo(() => readingInfo[2].icon, [readingInfo])

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
      {["Reading", "Label", "Action"]
        .map((v) => <TableColumn key={`header-${v}`}>{v}</TableColumn>)}
    </TableHeader>
  ), [])

  const entryRows = useCallback((type: NIRProbeReadingType.PD1 | NIRProbeReadingType.PD2) =>
    readings[type]
      .map(({data, type, label}, index) => (
        <TableRow key={index}>
          <TableCell key={"reading-"+index}>{data}</TableCell>
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
    )), [readings, deleteEntry, setLabel])

  const table = useCallback((type: NIRProbeReadingType.PD1 | NIRProbeReadingType.PD2) => (
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

  const PD1Table = useMemo(() => table(NIRProbeReadingType.PD1), [table])
  const PD2Table = useMemo(() => table(NIRProbeReadingType.PD2), [table])

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
          key={`${NIRProbeReadingType.PD1}`}
          title={
            <div className="flex items-center space-x-2">
              <PD1Icon/>
              <span>{readingInfo[1].name}</span>
            </div>
          }
        >
          {PD1Table}
        </Tab>
        <Tab
          key={`${NIRProbeReadingType.PD2}`}
          title={
            <div className="flex items-center space-x-2">
              <PD2Icon/>
              <span>{readingInfo[2].name}</span>
            </div>
          }
        >
          {PD2Table}
        </Tab>
      </Tabs>
    </div>
  );
}

export default NIRProbeFileTable;