import React, { useState, useEffect, useRef, useMemo, useCallback } from "react";
import { Button, CardProps, Chip } from "@nextui-org/react";

import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";

import ReactECharts from "echarts-for-react";
import { Download } from "react-feather";

interface KilnChartProps extends CardProps {

}

// Chart configuration
const chartConfig = {
  animation: false,
  grid: { left: 20, right: 20, top: 30, bottom: 30 },
  xAxis: {
    type: "time",
    name: "Time (s)",
    nameTextStyle: { color: "#fff" },
    axisLabel: { color: "#fff" },
  },
  yAxis: {
    name: "Temperature (°C)",
    nameTextStyle: { color: "#fff" },
    axisLabel: { color: "#fff" },
    splitLine: { show: false },
  },
  series: {
    name: "Kiln temperature",
    type: "line",
    showSymbol: false,
    lineStyle: { width: 3, color: "#992F7B" },
    markLine: {
      data: [{ yAxis: 100 }],
      symbol: "none",
      label: { show: false },
      lineStyle: {
        color: "rgba(255,255,255,0.6)",
        width: 2,
        type: "dashed",
      },
    },
  },
  tooltip: {
    trigger: "axis",
    axisPointer: {
      type: "line",
      lineStyle: { color: "rgba(255,255,255,0.5)" },
    },
    backgroundColor: "rgba(0,0,0,0.7)",
    borderWidth: 0,
    textStyle: { color: "#fff" },
    extraCssText: "border-radius:10px;",

    formatter: (params: { value: [number, number] }[]) => {
      const point = params[0].value;
      if (point) {
        const time = new Date(point[0]).toLocaleTimeString("en-GB", { hour12: false });
        const temp = point[1].toFixed(2);

        return `${time}<br/><b>${temp}°C</b>`;
      } else return ""

    },
  },
};

const KilnChart: React.FC<KilnChartProps> = () => {
  const [maxTemp, setMaxTemp] = useState(150);

  const [seriesData, setSeriesData] = useState({
    time: [] as number[],
    temp: [] as number[],
  });

  // set up to 'refresh' kilnData state
  const kilnData = useSelector(
    (state: RootState) => state.kilnData
  );

  const dataBifrost = useBifrost({ topic: RosTopic.KILN_DATA });

  useEffect(() => {
    dataBifrost.syncWithTopic(); // calling ros bridge to subscribe to topic
    // update max temps if current temps exceed them
    if (kilnData.temp[0] > maxTemp) {
      setMaxTemp(1.1 * kilnData.temp[0]);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [dataBifrost]);

  const addPoint = (currentData: number[], newValue: number) => {
    return [...currentData, newValue];
  };

  // Update existing data
  useEffect(() => {
    if (kilnData && kilnData.stamp) {
      setSeriesData((allData) => ({
        time: addPoint(allData.time, kilnData.stamp.sec * 1000 + kilnData.stamp.nanosec / 1_000_000),
        temp: addPoint(allData.temp, kilnData.temp[0]),
      }));
    } else return;
  }, [kilnData]);

  const chartRef = useRef<ReactECharts>(null);

  const chartOption = useMemo(() => {
    const data = seriesData.time.map((t, i) => [t, seriesData.temp[i]]);

    return {
      ...chartConfig,
      series: [
        {
          ...chartConfig.series,
          data,
        },
      ],
    };
  }, [seriesData]);


  const exportChart = useCallback(() => {
    const chartInstance = chartRef.current?.getEchartsInstance?.();

    if (chartInstance) {
      const url = chartInstance.getDataURL({
        type: "png",
        pixelRatio: 2,
        backgroundColor: "transparent",
      });

      const link = document.createElement("a");
      link.href = url;
      link.download = `kiln-temperature-${Date.now()}.png`;
      link.click();

    } else return;
  }, []);

  const resetTimescale = () => {
    setSeriesData({ time: [], temp: [] });
  };

  return (
    <>
      <div className="flex items-center justify-between gap-2">
        <div className="flex items-center gap-2">
          <Chip size="lg" variant="flat" radius="md" className="h-10" color="warning" classNames={{ content: "flex items-center gap-2" }}>
            <div className="text-sm">KILN</div>
            <div className="text-white">{kilnData?.temp?.[0] != null ? `${kilnData.temp[0].toFixed(2)}°C` : "--"}</div>
          </Chip>
          <Chip size="lg" variant="flat" radius="md" className="h-10" color="primary" classNames={{ content: "flex items-center gap-2" }}>
            <div className="text-sm">CONDENSER</div>
            <div className="text-white">{kilnData?.temp?.[1] != null ? `${kilnData.temp[1].toFixed(2)}°C` : "--"}</div>
          </Chip>
        </div>
        <div className="flex items-center gap-2">
          <Button size="sm" variant="flat" isIconOnly onPress={exportChart}>
            <Download size={16} />
          </Button>
          <Button size="sm" variant="flat" onPress={resetTimescale}>
            Reset timescale
          </Button>
        </div>
      </div>
      <div className="w-full" style={{ height: 240 }}>
        <ReactECharts
          ref={chartRef}
          option={chartOption}
          style={{ height: "100%", width: "100%" }}
          lazyUpdate={true}
          opts={{ renderer: "canvas" }}
        />
      </div>
    </>
  );
};

export default KilnChart;