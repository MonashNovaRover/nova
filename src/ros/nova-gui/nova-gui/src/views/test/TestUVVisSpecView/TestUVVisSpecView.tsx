import React, {useCallback, useMemo, useState} from "react";
import UVVisSpec from "../../../components/UVVisSpec/UVVisSpec.tsx";
import {ApexDataset} from "../../../components/SpectraDisplay/DataChart.tsx";
import {Button, Card, CardBody, CardHeader, Select, SelectItem} from "@nextui-org/react";
import ReactApexChart from "react-apexcharts";
import {useLocalStorage} from "../../../components/nir-probe/hooks/useLocalStorage.ts";
import {X} from "react-feather";


const TestUVVisSpecView: React.FC = () => {
  // Saved sets of data
  const [output, setOutputRaw] = useLocalStorage<ApexDataset>("uv-vis-spec-saved-data", []);
  // Sets of data selected for viewing
  const [selectedCharts, setSelectedCharts] = useState<Set<string>>(new Set([]));

  const setOutput = useCallback((points: number[][], name: string) => {
    const reduction = 4;

    // Downsample the data
    const results: number[][] = []
    for (let i = 0; i < Math.floor(points.length / reduction); i++) {
      let sumx = 0;
      let sumy = 0;

      for (let j = 0; j < reduction; j++) {
        sumx += points[i * reduction + j][0]
        sumy += points[i * reduction + j][1]
      }

      results.push([sumx/reduction, sumy/reduction]);
    }

    // Create a unique name for the data
    let currentIndex = 0;
    while (output.find((data) => data.name === name)) {
      // Remove any existing end
      if (name.endsWith(")")) {
        const split = name.split(' (')
        if (split.length >= 2) {
          const splitEnd = split.pop()
          currentIndex = +splitEnd!.substring(0, splitEnd!.length-1)

          name = split.join('')
        }
      }

      // Construct new name
      currentIndex += 1
      name = name + " (" + (currentIndex ).toString() + ")"
    }

    // Format into an apex dataset
    const dataset = {
      name: name,
      data: results
    }

    setOutputRaw(existing => [...existing, dataset])
    setSelectedCharts((existing: Set<string>) => new Set([...existing.values(), name]))
  }, [output, setOutputRaw])

  const onDeleteItem = useCallback((name: string) => {
    setOutputRaw((current: ApexDataset) => current.filter((data) => data.name !== name));
  }, [setOutputRaw])

  const selectedOutput = useMemo(() => (
    output.filter(({name}) => selectedCharts.has(name))
  ), [output, selectedCharts])

  const options: ApexCharts.ApexOptions = {
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
    }
  }

  return <div className="m-3 grid grid-cols-2 gap-3">
    <div className="flex flex-col">
      <UVVisSpec onSave={setOutput}/>
    </div>
    <div className="flex flex-col">
      <Card>
        <CardHeader className="flex flex-row gap-3 text-nowrap items-center">
          <div>UV Vis Spec Saved Graphs</div>
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
            {output.map(({name}) => (
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
    </div>
  </div>;
};

export default TestUVVisSpecView;
