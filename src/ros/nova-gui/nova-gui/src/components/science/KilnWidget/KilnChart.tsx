import React, { useState, useEffect, useRef, useMemo } from "react";
import { Card, CardProps } from "@nextui-org/react";

import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";

import ReactECharts from "echarts-for-react";

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
};

const KilnChart: React.FC<KilnChartProps> = (props) => {
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
  }, [chartConfig, seriesData]);

  return (
    <Card {...props} className="space-y-3 p-3">
      <div className="w-full" style={{ height: 280 }}>
        <ReactECharts
          ref={chartRef}
          option={chartOption}
          style={{ height: "100%", width: "100%" }}
          lazyUpdate={true}
          opts={{ renderer: "canvas" }}
        />
      </div>
    </Card>
  );
};

export default KilnChart;