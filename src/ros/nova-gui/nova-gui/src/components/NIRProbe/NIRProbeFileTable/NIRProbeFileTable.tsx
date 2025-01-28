import {
  Button,
  CardProps,
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
import {ISpaceResourcesEntry, NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";
import {Droplet, Square, Trash2} from "react-feather";
import {useAbsorbance} from "../NIRProbeCalibration/NIRCalibration.ts";

export interface NIRProbeFileTableProps extends CardProps {
  showAdvanced : boolean,
}

const NIRProbeFileTable: React.FC<NIRProbeFileTableProps> = ({
  showAdvanced
}) => {

  // NIR Probe readings data corresponding to the currently selected site.
  const [readings, setReadings] = useNIRSiteData();
  const absorbance = useAbsorbance();

  const deleteEntry = useCallback((index: number) => {
    setReadings(readings.filter((_, i) => i !== index))
  }, [readings, setReadings]);

  const tableHeader = useCallback(() => {
    const cols = showAdvanced ? ["Reading (Difference)", "Absorbance", "Label", "Action"]
      : ["Reading (Difference)", "Absorbance", "Action"];

    return (
      <TableHeader>
        {cols.map((v) => <TableColumn key={`header-${v}`}>{v}</TableColumn>)}
      </TableHeader>
    )
  }, [showAdvanced])

  const entryRows = useCallback((type: NIRProbeReadingType) => readings
    .filter((v: ISpaceResourcesEntry) => v.type == type)
    .map(({data, type, label}, index) => {
      const extras = showAdvanced ? [<TableCell key="label">{label}</TableCell>] : [];
      const cells = [
        <TableCell key={"reading-"+index}>{data}</TableCell>,
        <TableCell key={"absorbance-"+index}>{absorbance(type, data).toFixed(4)}</TableCell>,
        ...extras,
        <TableCell key="action">
          <Button onPress={() => deleteEntry(index)}
                  size="sm" color="danger" variant="light" className="w-full" key="delete-button">
            <Trash2/>
          </Button>
        </TableCell>,
      ];

      return (<TableRow key={index}>
        {cells}
      </TableRow>);
  }), [readings, deleteEntry, showAdvanced])

  const table = useCallback((type: NIRProbeReadingType) => (
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