import React, {useState, useMemo} from "react";
import {Card, CardHeader, CardBody, Select, SelectItem} from "@nextui-org/react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import {ApexDataset} from "../SpectraDisplay/DataChart.tsx";
import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";

interface DualGraphWidgetProps {
  site: Site;
  title: string;
}

const DualGraphWidget: React.FC<DualGraphWidgetProps> = ({site, title}) => {
  const [allSpectra, _] = useLocalStorage<ApexDataset>("uv-vis-spec-saved-data", []);
  const [selectedGraph1, setSelectedGraph1] = useState<string>("");
  const [selectedGraph2, setSelectedGraph2] = useState<string>("");

  // Filter UV-Vis spectra by site identifier
  const siteIdentifier = `S${site + 1}`;
  const filteredSpectra = useMemo(() =>
    allSpectra.filter(spec => spec.name.includes(siteIdentifier)),
    [allSpectra, siteIdentifier]
  );

  // Get selected graphs
  const graph1 = useMemo(() =>
    filteredSpectra.find(spec => spec.name === selectedGraph1),
    [filteredSpectra, selectedGraph1]
  );

  const graph2 = useMemo(() =>
    filteredSpectra.find(spec => spec.name === selectedGraph2),
    [filteredSpectra, selectedGraph2]
  );

  // Chart options
  const createChartOptions = (graphName: string): ApexOptions => ({
    chart: {
      id: `uv-vis-${graphName}`,
      background: "transparent",
      toolbar: {
        show: false
      }
    },
    stroke: {
      curve: 'smooth',
    },
    xaxis: {
      type: 'numeric',
      tickAmount: 10,
    },
    yaxis: {
      show: true
    },
    theme: {
      mode: "dark",
    },
    title: {
      text: graphName,
      style: {
        fontSize: '12px',
      }
    }
  });

  return (
    <Card>
      <CardHeader className="text-h1 pb-0">
        {title}
      </CardHeader>
      <CardBody>
        <div className="flex flex-col gap-3">
          {/* Graph Selectors */}
          <div className="grid grid-cols-2 gap-3">
            <Select
              size="sm"
              label="Graph 1"
              selectedKeys={selectedGraph1 ? [selectedGraph1] : []}
              onSelectionChange={(keys) => {
                const key = Array.from(keys)[0] as string;
                setSelectedGraph1(key || "");
              }}
            >
              {filteredSpectra.map((spec) => (
                <SelectItem key={spec.name} value={spec.name}>
                  {spec.name}
                </SelectItem>
              ))}
            </Select>

            <Select
              size="sm"
              label="Graph 2"
              selectedKeys={selectedGraph2 ? [selectedGraph2] : []}
              onSelectionChange={(keys) => {
                const key = Array.from(keys)[0] as string;
                setSelectedGraph2(key || "");
              }}
            >
              {filteredSpectra.map((spec) => (
                <SelectItem key={spec.name} value={spec.name}>
                  {spec.name}
                </SelectItem>
              ))}
            </Select>
          </div>

          {/* Graphs Side by Side */}
          <div className="grid grid-cols-2 gap-3">
            <div>
              {graph1 ? (
                <ReactApexChart
                  options={createChartOptions(graph1.name)}
                  series={[{name: graph1.name, data: graph1.data}]}
                  type="line"
                  height={250}
                />
              ) : (
                <div className="h-[250px] flex items-center justify-center text-default-500">
                  Select Graph 1
                </div>
              )}
            </div>

            <div>
              {graph2 ? (
                <ReactApexChart
                  options={createChartOptions(graph2.name)}
                  series={[{name: graph2.name, data: graph2.data}]}
                  type="line"
                  height={250}
                />
              ) : (
                <div className="h-[250px] flex items-center justify-center text-default-500">
                  Select Graph 2
                </div>
              )}
            </div>
          </div>
        </div>
      </CardBody>
    </Card>
  );
};

export default DualGraphWidget;
