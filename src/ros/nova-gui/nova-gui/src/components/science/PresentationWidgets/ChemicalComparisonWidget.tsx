import React, {useState, useMemo} from "react";
import {Card, CardHeader, CardBody, Button} from "@nextui-org/react";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import {ApexDataset} from "../SpectraDisplay/DataChart.tsx";
import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";
import {CHEMICALS, Chemical, CHEMICAL_MEASUREMENT_TYPE, MeasurementType} from "../UVVisSpec/chemicalConfig.ts";
import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";

export type Spectrometer = "SL" | "SR";

interface ChemicalComparisonWidgetProps {
  spectrometer: Spectrometer;
  columnLabel: string;
  onSwap?: () => void;
  currentMapping?: string;
}

const ChemicalComparisonWidget: React.FC<ChemicalComparisonWidgetProps> = ({
  spectrometer,
  columnLabel,
  onSwap,
  currentMapping
}) => {
  const [allSpectra, _] = useLocalStorage<ApexDataset>("uv-vis-spec-saved-data", []);
  const [selectedIndex, setSelectedIndex] = useState<number>(0);
  const selectedChemical: Chemical = CHEMICALS[selectedIndex];

  // Filter spectra by spectrometer and chemical
  const filteredSpectra = useMemo(() => {
    return allSpectra.filter(spec => {
      const name = spec.name;
      // Check if it contains the chemical name
      if (!name.includes(selectedChemical)) return false;

      // Include if it's for this spectrometer OR if it's a blank (neg/pos/Blank without spectrometer label)
      const isForSpectrometer = name.includes(spectrometer);
      const isBlank = (name.includes("neg") || name.includes("pos") || name.includes("Blank"))
        && !name.includes("SL") && !name.includes("SR");

      return isForSpectrometer || isBlank;
    });
  }, [allSpectra, selectedChemical, spectrometer]);

  // Chart options - y-axis label changes based on chemical measurement type
  const chartOptions: ApexOptions = useMemo(() => {
    const measurementType = CHEMICAL_MEASUREMENT_TYPE[selectedChemical];

    const yAxisLabel = measurementType === MeasurementType.INTENSITY
      ? 'Intensity (normalized)'
      : 'Absorbance';

    return {
      chart: {
        id: `chemical-comparison-${spectrometer}`,
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
  }, [spectrometer, selectedChemical]);

  return (
    <Card>
      <CardHeader className="flex flex-col items-start pb-2">
        <div className="flex flex-row items-center justify-between w-full">
          <div className="text-h1">{columnLabel} Chemical Comparison</div>
          {onSwap && currentMapping && (
            <div className="flex items-center gap-3">
              <span className="text-default-500">{currentMapping}</span>
              <Button
                variant="flat"
                isIconOnly
                onPress={onSwap}
                title="Swap spectrometer columns"
              >
                <span className="text-xl">&#8644;</span>
              </Button>
            </div>
          )}
        </div>
        <SegmentedPicker
          selectedIndex={selectedIndex}
          onIndexChange={setSelectedIndex}
          className="mt-2 w-full"
          size="sm"
          fullWidth
          color="secondary"
        >
          {CHEMICALS.map((chemical) => chemical)}
        </SegmentedPicker>
      </CardHeader>
      <CardBody>
        {filteredSpectra.length > 0 ? (
          <ReactApexChart
            options={chartOptions}
            series={filteredSpectra.map(spec => ({
              name: spec.name,
              data: spec.data
            }))}
            type="line"
            height={500}
          />
        ) : (
          <div className="h-[350px] flex items-center justify-center text-default-500">
            No data available for {selectedChemical} ({spectrometer})
          </div>
        )}
      </CardBody>
    </Card>
  );
};

export default ChemicalComparisonWidget;
