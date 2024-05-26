import GenericGraphComparisonWidget
  from "../../../components/GenericGraphComparisonWidget/GenericGraphComparisonWidget.tsx";
import React, {useState} from "react";
import {useLocalStorage} from "../../../components/nir-probe/hooks/useLocalStorage.ts";
import {ApexDataset} from "../../../components/SpectraDisplay/DataChart.tsx";
import RamanSpec from "../../../components/RamanSpec/RamanSpec.tsx";


const TestRamanView: React.FC = () => {
  const [selectedGraphs, setSelectedGraphs] = useState<Set<string>>(new Set([]));
  const [graphs, setGraphs] = useLocalStorage<ApexDataset>("raman-spec-graphs", []);

  return <div className="grid grid-flow-col auto-cols-fr grid-cols-2">
    <div className="flex flex-col">
      <RamanSpec/>
    </div>
    <div className="flex flex-col">
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
