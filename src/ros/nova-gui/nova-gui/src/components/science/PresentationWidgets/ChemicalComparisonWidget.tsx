import React, {useMemo} from "react";
import {Card, CardHeader, CardBody, Button} from "@nextui-org/react";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import {ApexDataset} from "../SpectraDisplay/DataChart.tsx";
import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";
import {CHEMICALS, Chemical, CHEMICAL_DISPLAY_NAMES, CHEMICAL_MEASUREMENT_TYPE, MeasurementType} from "../UVVisSpec/chemicalConfig.ts";
import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";

export type Spectrometer = "SL" | "SR";

const getSeriesColor = (name: string): string => {
  if (name.includes("neg")) return "#f31260"; // danger - red
  if (name.includes("pos")) return "#2ac776"; // success - green
  if (name.toLowerCase().includes("sl") || name.toLowerCase().includes("sr")) return "#3eb1cf"; // blue
  return "#F770AD"; // primary - pink fallback
};

const getSeriesWidth = (name: string): number => {
  if (name.includes("neg") || name.includes("pos")) return 6; // thick
  return 2; // normal for samples
};

interface ChemicalComparisonWidgetProps {
  spectrometer: Spectrometer;
  columnLabel: string;
  onSwap?: () => void;
  currentMapping?: string;
  selectedIndex: number;
  onIndexChange: (index: number) => void;
  yAxisMin: number;
  yAxisMax: number;
}

const ChemicalComparisonWidget: React.FC<ChemicalComparisonWidgetProps> = ({
  spectrometer,
  columnLabel,
  onSwap,
  currentMapping,
  selectedIndex,
  onIndexChange,
  yAxisMin,
  yAxisMax
}) => {
  const [allSpectra, _] = useLocalStorage<ApexDataset>("uv-vis-spec-saved-data", []);
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

  const seriesColors = useMemo(() => {
    return filteredSpectra.map(spec => getSeriesColor(spec.name));
  }, [filteredSpectra]);

  const seriesWidths = useMemo(() => {
    return filteredSpectra.map(spec => getSeriesWidth(spec.name));
  }, [filteredSpectra]);

  // Chart options - y-axis label changes based on chemical measurement type
  const chartOptions: ApexOptions = useMemo(() => {
    const measurementType = CHEMICAL_MEASUREMENT_TYPE[selectedChemical];

    const yAxisLabel = measurementType === MeasurementType.INTENSITY
      ? 'Intensity (normalized)'
      : 'Absorbance';

    return {
      colors: seriesColors,
      chart: {
        id: `chemical-comparison-${spectrometer}`,
        background: "transparent",
        toolbar: {
          show: false
        }
      },
      stroke: {
        curve: 'smooth',
        width: seriesWidths,
      },
      xaxis: {
        type: 'numeric',
        tickAmount: 10,
        title: {
          text: 'Wavelength (nm)',
          style: {
            fontSize: '18px',
          }
        },
        labels: {
          style: {
            fontSize: '16px',
          }
        }
      },
      yaxis: {
        min: yAxisMin,
        max: yAxisMax,
        tickAmount: 5,
        show: true,
        title: {
          text: yAxisLabel,
          style: {
            fontSize: '18px',
          }
        },
        labels: {
          formatter: (value: number) => value.toFixed(2),
          style: {
            fontSize: '16px',
          }
        }
      },
      theme: {
        mode: "dark",
      },
      legend: {
        show: true,
        position: 'bottom',
        fontSize: '18px',
      },
    };
  }, [spectrometer, selectedChemical, yAxisMin, yAxisMax, seriesColors, seriesWidths]);

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
          onIndexChange={onIndexChange}
          className="mt-2 w-full"
          size="sm"
          fullWidth
          color="secondary"
        >
          {CHEMICALS.map((chemical) => CHEMICAL_DISPLAY_NAMES[chemical])}
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
