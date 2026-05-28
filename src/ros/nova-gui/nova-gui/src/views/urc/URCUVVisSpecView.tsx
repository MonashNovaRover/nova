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
  // Toggle for overwriting duplicates (default: true = overwrite)
  const [overwriteDuplicates, setOverwriteDuplicates] = useState(true);


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

    let finalName = name;

    if (overwriteDuplicates) {
      // Replace existing spectrum with same name
      const dataset = { name: finalName, data: results };
      setOutputRaw(existing => {
        const filtered = existing.filter(spec => spec.name !== name);
        return [...filtered, dataset];
      });
    } else {
      // Use unique name logic (append (1), (2), etc.)
      finalName = getUniqueName(name, output.map(({name}) => name));
      const dataset = { name: finalName, data: results };
      setOutputRaw(existing => [...existing, dataset]);
    }

    setSelectedCharts((existing: Set<string>) => new Set([...existing.values(), finalName]))
  }, [output, setOutputRaw, overwriteDuplicates])

  return <div className="flex flex-col gap-3">
      <UVVisSpec
        onSave={setOutput}
        overwriteDuplicates={overwriteDuplicates}
        onOverwriteToggle={setOverwriteDuplicates}
      />
      <div className="flex flex-col overflow-hidden">
        <GenericGraphComparisonWidget
          graphs={output}
          setGraphs={setOutputRaw}
          selectedCharts={selectedCharts}
          setSelectedCharts={setSelectedCharts}
          title={"UV Vis Spec Saved Graphs"}
          onDeleteAll={() => {
            setOutputRaw([]);
            setSelectedCharts(new Set([]));
          }}
        >
        </GenericGraphComparisonWidget>
      </div>
    </div>
};

export default URCUVVisSpecView;
