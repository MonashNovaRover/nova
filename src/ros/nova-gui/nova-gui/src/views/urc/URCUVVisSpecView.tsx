import React, {useCallback, useState} from "react";
import UVVisSpec from "../../components/science/UVVisSpec/UVVisSpec.tsx";
import {ApexDataset} from "../../components/science/SpectraDisplay/DataChart.tsx";
import {useLocalStorage} from "../../hooks/useLocalStorage.ts";
import GenericGraphComparisonWidget, {getUniqueName} from "../../components/shared/widgets/GenericGraphComparisonWidget/GenericGraphComparisonWidget.tsx";


const URCUVVisSpecView: React.FC = () => {
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

    name = getUniqueName(name, output.map(({name}) => name));

    // Format into an apex dataset
    const dataset = {
      name: name,
      data: results
    }

    setOutputRaw(existing => [...existing, dataset])
    setSelectedCharts((existing: Set<string>) => new Set([...existing.values(), name]))
  }, [output, setOutputRaw])

  return <div className="grid grid-rows-2 gap-3">
    <div className="flex flex-col">
      <UVVisSpec onSave={setOutput}/>
    </div>
    <div className="flex flex-col overflow-hidden">
      <GenericGraphComparisonWidget
        graphs={output}
        setGraphs={setOutputRaw}
        selectedCharts={selectedCharts}
        setSelectedCharts={setSelectedCharts}
        title={"UV Vis Spec Saved Graphs"}
      >
      </GenericGraphComparisonWidget>
    </div>
  </div>;
};

export default URCUVVisSpecView;
