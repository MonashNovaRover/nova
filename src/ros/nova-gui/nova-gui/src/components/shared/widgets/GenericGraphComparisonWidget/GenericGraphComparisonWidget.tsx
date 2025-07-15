/**
 * A sloppy component for displaying a bunch of graphs. It's not great, but hey, it works.
 * Author: Bailey Chessum
 */
import React, {memo, ReactNode, useCallback, useMemo} from "react";
import {ApexDataset} from "../../../science/SpectraDisplay/DataChart.tsx";
import {ApexOptions} from "apexcharts";
import {Button, Card, CardBody, CardHeader, Select, SelectItem} from "@nextui-org/react";
import {X} from "react-feather";
import ReactApexChart from "react-apexcharts";


export interface GenericGraphComparisonWidgetProps {
  selectedCharts: Set<string>,
  setSelectedCharts: React.Dispatch<React.SetStateAction<Set<string>>>,

  graphs: ApexDataset,
  setGraphs: (newValue: (ApexDataset | ((previousValue: ApexDataset) => ApexDataset))) => void,

  options?: Partial<ApexOptions>

  title: ReactNode
}


export function getUniqueName(name: string, existingNames: string[]) {
  let uniqueName = name;

  // Create a unique name for the data
  let currentIndex = 0;
  while (existingNames.find((v) => v === uniqueName)) {
    // Remove any existing number suffix
    const regex = new RegExp(` \\(${currentIndex.toString()}\\)$`);
    uniqueName = uniqueName.replace(regex, '');

    // Append an updated number suffix
    currentIndex++;
    uniqueName += ` (${currentIndex.toString()})`;
  }

  return uniqueName;
}


const GenericGraphComparisonWidgetUnmemoed: React.FC<GenericGraphComparisonWidgetProps> = (props) => {
  const graphs = props.graphs;
  const setGraphs = props.setGraphs
  const selectedCharts = props.selectedCharts;
  const setSelectedCharts = props.setSelectedCharts;

  // The data filtered to only those selected
  const selectedOutput = useMemo(() => (
    graphs.filter(({name}) => selectedCharts.has(name))
  ), [graphs, selectedCharts])

  // Callback to remove a graph
  const onDeleteItem = useCallback((name: string) => {
    setGraphs((current: ApexDataset) => current.filter((data) => data.name !== name));
  }, [setGraphs])

  // The options for formatting the chart
  const options: ApexCharts.ApexOptions = useMemo(() => ({
    series: selectedOutput.map((data) => ({
      data: data.data,
      name: data.name,
      type: "line",
    })),
    stroke: {
      curve: 'smooth',
    },
    chart: {
      id: "uv-vis-out",
      background: "transparent"
    },
    xaxis: {
      type: 'numeric',
      tickAmount: 10,
      stepSize: 50,
    },
    yaxis: {
      show: false
    },
    theme: {
      mode: "dark",
    },
    ...props.options
  }), [props.options, selectedOutput])

  return (
    <Card>
      <CardHeader className="flex flex-row gap-3 text-nowrap items-center">
        <div>{props.title}</div>
        <Select
          size="sm"
          selectionMode="multiple"
          selectedKeys={selectedCharts}
          onSelectionChange={v => setSelectedCharts(v as Set<string>)}
          aria-label="UV Vis Spec Saved Graphs"
          renderValue={(items) => (
            <div className="flex flex-row gap-1.5">
              {items.map((item, index) => (
                <div key={index}>{`${item.key}`},</div>
              ))}
            </div>
          )}
        >
          {graphs.map(({name}) => (
            <SelectItem key={name} className="py-0">
              <div className="flex flex-row items-center">
                <div className="flex-grow">{name}</div>
                <Button isIconOnly={true} size="sm" color="danger" variant="light" onPress={() => onDeleteItem(name)}>
                  <X/>
                </Button>
              </div>
            </SelectItem>
          ))}
        </Select>
      </CardHeader>
      <CardBody>
        <ReactApexChart options={options} type="line" series={selectedOutput}></ReactApexChart>
      </CardBody>
    </Card>
  );
}

const GenericGraphComparisonWidget = memo(GenericGraphComparisonWidgetUnmemoed);
export default GenericGraphComparisonWidget;
