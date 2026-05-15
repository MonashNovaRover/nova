import React, {useState, useMemo} from "react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import {ApexDataset} from "../SpectraDisplay/DataChart.tsx";
import GenericGraphComparisonWidget from "../../shared/widgets/GenericGraphComparisonWidget/GenericGraphComparisonWidget.tsx";

const UVVisComparisonWidget: React.FC = () => {
  const [currentSite, _] = useGenericStore<Site>("currentSite");
  const [allSpectra, setAllSpectra] = useLocalStorage<ApexDataset>("uv-vis-spec-saved-data", []);
  const [selectedCharts, setSelectedCharts] = useState<Set<string>>(new Set([]));

  // Filter spectra by site identifier in name (S1, S2, S3, S4)
  const siteIdentifier = `S${currentSite + 1}`;
  const filteredSpectra = useMemo(() =>
    allSpectra.filter(spec => spec.name.includes(siteIdentifier)),
    [allSpectra, siteIdentifier]
  );

  return (
    <GenericGraphComparisonWidget
      graphs={filteredSpectra}
      setGraphs={setAllSpectra}
      selectedCharts={selectedCharts}
      setSelectedCharts={setSelectedCharts}
      title={`UV-Vis Spectra - Site ${currentSite + 1}`}
    />
  );
};

export default UVVisComparisonWidget;
