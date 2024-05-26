import GenericGraphComparisonWidget
  , {getUniqueName} from "../../../components/GenericGraphComparisonWidget/GenericGraphComparisonWidget.tsx";
import React, {useCallback, useState} from "react";
import {useLocalStorage} from "../../../components/nir-probe/hooks/useLocalStorage.ts";
import {ApexDataset} from "../../../components/SpectraDisplay/DataChart.tsx";
import RamanSpec from "../../../components/RamanSpec/RamanSpec.tsx";


const TestRamanView: React.FC = () => {
  const [selectedGraphs, setSelectedGraphs] = useState<Set<string>>(new Set([]));
  const [graphs, setGraphs] = useLocalStorage<ApexDataset>("raman-spec-graphs", []);

  const onSave = useCallback((data: number[][], name: string) => {
    const uniqueName = getUniqueName(name, graphs.map((data) => data.name));

    const graph = {
      name: uniqueName,
      data: data
    }

    setGraphs([...graphs, graph]);
    setSelectedGraphs(new Set([...selectedGraphs.values(), uniqueName]))
  }, [graphs, setGraphs]);

  return <div className="grid grid-flow-col auto-cols-fr grid-cols-1 m-3">
    <div className="flex flex-col">
      <RamanSpec onSave={onSave}/>
      <GenericGraphComparisonWidget
        graphs={graphs}
        setGraphs={setGraphs}
        selectedCharts={selectedGraphs}
        setSelectedCharts={setSelectedGraphs}
        title={"RAMAN Spec Saved Graphs"}
      >
      </GenericGraphComparisonWidget>
    </div>
  </div>;
};

export default TestRamanView;
