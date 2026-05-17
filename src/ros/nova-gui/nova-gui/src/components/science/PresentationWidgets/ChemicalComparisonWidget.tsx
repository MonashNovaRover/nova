import React, {useState, useMemo} from "react";
import {Card, CardHeader, CardBody, Select, SelectItem} from "@nextui-org/react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import {ApexDataset} from "../SpectraDisplay/DataChart.tsx";
import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";
import {CHEMICALS, Chemical, CHEMICAL_MEASUREMENT_TYPE, MeasurementType} from "../UVVisSpec/chemicalConfig.ts";

interface ChemicalComparisonWidgetProps {
  site: Site;
}

const ChemicalComparisonWidget: React.FC<ChemicalComparisonWidgetProps> = ({site}) => {
  const [allSpectra, _] = useLocalStorage<ApexDataset>("uv-vis-spec-saved-data", []);
  const [selectedChemical, setSelectedChemical] = useState<Chemical | "">("Nile Red");

  // Filter spectra by site and chemical
  const siteIdentifier = `S${site + 1}`;
  const filteredSpectra = useMemo(() => {
    if (!selectedChemical) return [];

    return allSpectra.filter(spec => {
      const name = spec.name;
      // Check if it contains the chemical name
      if (!name.includes(selectedChemical)) return false;

      // Include if it's for this site OR if it's a blank (neg/pos/Blank without site)
      const isForSite = name.includes(siteIdentifier);
      const isBlank = (name.includes("neg") || name.includes("pos") || name.includes("Blank"))
        && !name.includes("S1") && !name.includes("S2") && !name.includes("S3") && !name.includes("S4");

      return isForSite || isBlank;
    });
  }, [allSpectra, selectedChemical, siteIdentifier]);

  // Chart options - y-axis label changes based on chemical measurement type
  const chartOptions: ApexOptions = useMemo(() => {
    const measurementType = selectedChemical
      ? CHEMICAL_MEASUREMENT_TYPE[selectedChemical]
      : MeasurementType.ABSORBANCE;

    const yAxisLabel = measurementType === MeasurementType.INTENSITY
      ? 'Intensity (normalized)'
      : 'Absorbance';

    return {
      chart: {
        id: `chemical-comparison-site-${site}`,
        background: "transparent",
        toolbar: {
          show: false
        }
      },
      stroke: {
        curve: 'smooth',
        width: 2,
      },
      xaxis: {
        type: 'numeric',
        tickAmount: 10,
        title: {
          text: 'Wavelength (nm)',
          style: {
            fontSize: '12px',
          }
        }
      },
      yaxis: {
        show: true,
        title: {
          text: yAxisLabel,
          style: {
            fontSize: '12px',
          }
        }
      },
      theme: {
        mode: "dark",
      },
      legend: {
        show: true,
        position: 'bottom',
        fontSize: '10px',
      },
    };
  }, [site, selectedChemical]);

  return (
    <Card>
      <CardHeader className="flex flex-col items-start pb-2">
        <div className="text-h1">Site {site + 1} - Chemical Comparison</div>
        <Select
          size="sm"
          label="Select Chemical"
          selectedKeys={selectedChemical ? [selectedChemical] : []}
          onSelectionChange={(keys) => {
            const key = Array.from(keys)[0] as Chemical;
            setSelectedChemical(key || "");
          }}
          className="max-w-xs mt-2"
        >
          {CHEMICALS.map((chemical) => (
            <SelectItem key={chemical} value={chemical}>
              {chemical}
            </SelectItem>
          ))}
        </Select>
      </CardHeader>
      <CardBody>
        {selectedChemical && filteredSpectra.length > 0 ? (
          <ReactApexChart
            options={chartOptions}
            series={filteredSpectra.map(spec => ({
              name: spec.name,
              data: spec.data
            }))}
            type="line"
            height={350}
          />
        ) : (
          <div className="h-[350px] flex items-center justify-center text-default-500">
            {selectedChemical
              ? `No data available for ${selectedChemical} at Site ${site + 1}`
              : "Select a chemical to view comparison"
            }
          </div>
        )}
      </CardBody>
    </Card>
  );
};

export default ChemicalComparisonWidget;
