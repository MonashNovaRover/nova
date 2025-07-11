import GenericGraphComparisonWidget
  , {getUniqueName} from "../../components/shared/widgets/GenericGraphComparisonWidget/GenericGraphComparisonWidget.tsx";
import React, {useCallback, useState} from "react";
import {useLocalStorage} from "../../hooks/useLocalStorage.ts";
import {ApexDataset} from "../../components/science/SpectraDisplay/DataChart.tsx";
import RamanSpec from "../../components/science/RamanSpec/RamanSpec.tsx";


const URCRamanView: React.FC = () => {
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
  }, [graphs, selectedGraphs, setGraphs]);

  return (
    <div className="grid w-full h-full gap-3 p-3 grid-cols-4">
        <RamanSpec className="row-start-1 w-full col-span-4 row-span-1" onSave={onSave}/>
        <div className="row-start-2 w-full col-span-3 row-span-1">
          <GenericGraphComparisonWidget
            graphs={graphs}
            setGraphs={setGraphs}
            selectedCharts={selectedGraphs}
            setSelectedCharts={setSelectedGraphs}
            title={"RAMAN Spec Saved Graphs"}
          />
        </div>
    </div>
  );
};

export default URCRamanView;
