import { useMemo } from "react";
import ReactECharts from "echarts-for-react";
import { PotentiostatReading } from "./potentiostatStorage.ts";

const CHART_COLORS = {
  channel1: "#F770AD", // blue
  channel2: "#AD2D67", // purple
};

export interface PotentiostatChartProps {
  channel1: PotentiostatReading[];
  channel2: PotentiostatReading[];
  height?: number;
}

export const PotentiostatChart = ({
  channel1,
  channel2,
  height = 300,
}: PotentiostatChartProps) => {
  const chartOption = useMemo(() => {
    // Convert stored data to scatter plot format [current, voltage]
    const channel1Data = channel1.map((r) => [r.current, r.voltage]);
    const channel2Data = channel2.map((r) => [r.current, r.voltage]);

    const fontSize = 16;

    return {
      animation: false,
      grid: { left: 50, right: 20, top: 30, bottom: 40 },
      xAxis: {
        type: "value",
        name: "Current",
        nameLocation: "center",
        nameGap: 25,
        nameTextStyle: { color: "#fff", fontSize },
        axisLabel: { color: "#fff", fontSize },
        splitLine: { lineStyle: { color: "rgba(255,255,255,0.1)" } },
      },
      yAxis: {
        type: "value",
        name: "Voltage",
        nameTextStyle: { color: "#fff", fontSize },
        axisLabel: { color: "#fff", fontSize },
        splitLine: { lineStyle: { color: "rgba(255,255,255,0.1)" } },
      },
      legend: {
        show: true,
        top: 0,
        textStyle: { color: "#fff", fontSize },
      },
      tooltip: {
        trigger: "item",
        backgroundColor: "rgba(0,0,0,0.7)",
        borderWidth: 0,
        textStyle: { color: "#fff", fontSize },
        formatter: (params: { seriesName: string; value: [number, number] }) => {
          const [current, voltage] = params.value;
          return `${params.seriesName}<br/>Current: ${current.toFixed(4)}<br/>Voltage: ${voltage.toFixed(2)}`;
        },
      },
      series: [
        {
          name: "Channel 1",
          type: "scatter",
          data: channel1Data,
          symbolSize: 6,
          itemStyle: { color: CHART_COLORS.channel1 },
        },
        {
          name: "Channel 2",
          type: "scatter",
          data: channel2Data,
          symbolSize: 6,
          itemStyle: { color: CHART_COLORS.channel2 },
        },
      ],
    };
  }, [channel1, channel2]);

  return (
    <div className="w-full" style={{ height }}>
      <ReactECharts
        option={chartOption}
        style={{ height: "100%", width: "100%" }}
        lazyUpdate={true}
        opts={{ renderer: "canvas" }}
      />
    </div>
  );
};

export { CHART_COLORS };
