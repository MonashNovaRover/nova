import React, {useState, useMemo} from "react";
import {Card, CardHeader, CardBody} from "@nextui-org/react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import {ApexDataset} from "../SpectraDisplay/DataChart.tsx";
import GenericGraphComparisonWidget from "../../shared/widgets/GenericGraphComparisonWidget/GenericGraphComparisonWidget.tsx";

interface SiteComparisonWidgetProps {
  site: Site;
}

const SiteComparisonWidget: React.FC<SiteComparisonWidgetProps> = ({site}) => {
  const [allSpectra, setAllSpectra] = useLocalStorage<ApexDataset>("uv-vis-spec-saved-data", []);
  const [selectedCharts, setSelectedCharts] = useState<Set<string>>(new Set([]));

  // Filter UV-Vis spectra by site identifier
  const siteIdentifier = `S${site + 1}`;
  const filteredSpectra = useMemo(() =>
    allSpectra.filter(spec => spec.name.includes(siteIdentifier)),
    [allSpectra, siteIdentifier]
  );

  return (
    <Card>
      <CardHeader className="text-h1 pb-0">
        UV-Vis Spectra - Site {site + 1}
      </CardHeader>
      <CardBody>
        {filteredSpectra.length === 0 ? (
          <p className="text-default-500 text-center py-4">No UV-Vis spectra saved</p>
        ) : (
          <GenericGraphComparisonWidget
            graphs={filteredSpectra}
            setGraphs={setAllSpectra}
            selectedCharts={selectedCharts}
            setSelectedCharts={setSelectedCharts}
            title=""
          />
        )}
      </CardBody>
    </Card>
  );
};

export default SiteComparisonWidget;
