import { useMemo } from "react";
import ReactECharts from "echarts-for-react";
import { PotentiostatReading } from "./potentiostatStorage.ts";

const CHART_COLORS = {
  channel1: "#F770AD", // pink
  channel2: "#AD2D67", // purple
};

type WidgetMode = "measurement" | "calibration";

export interface PotentiostatChartProps {
  channel1: PotentiostatReading[];
  channel2: PotentiostatReading[];
  mode: WidgetMode;
  height?: number;
}

export const PotentiostatChart = ({
  channel1,
  channel2,
  mode,
  height = 300,
}: PotentiostatChartProps) => {
  const chartOption = useMemo(() => {
    // Convert stored data to scatter plot format [voltage, current]
    const channel1Data = channel1.map((r) => [r.voltage, r.current]);
    const channel2Data = channel2.map((r) => [r.voltage, r.current]);

    const fontSize = 16;

    // Build series array
    const series: object[] = [
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
    ];

    return {
      animation: false,
      grid: { left: 50, right: 20, top: mode === "calibration" ? 50 : 30, bottom: 40 },
      title: mode === "calibration"
        ? {
            show: true,
            text: "Calibration Mode - Data not shown on chart",
            left: "center",
            top: 25,
            textStyle: { color: "#FFA500", fontSize: 14 },
          }
        : {
            show: false,
            text: "",
          },
      xAxis: {
        type: "value",
        name: "Voltage (v)",
        nameLocation: "center",
        nameGap: 25,
        nameTextStyle: { color: "#fff", fontSize },
        axisLabel: { color: "#fff", fontSize },
        splitLine: { lineStyle: { color: "rgba(255,255,255,0.1)" } },
      },
      yAxis: {
        type: "value",
        name: "Current (mA)",
        nameLocation: "middle",
        nameGap: 25,
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
          const [voltage, current] = params.value;
          return `${params.seriesName}<br/>Voltage: ${voltage.toFixed(2)}<br/>Current: ${current.toFixed(4)}`;
        },
      },
      series,
    };
  }, [channel1, channel2, mode]);

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
