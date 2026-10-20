import React, { useState, useEffect, useRef, useMemo, useCallback } from "react";
import { Button, Chip } from "@heroui/react";

import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";

import ReactECharts from "echarts-for-react";
import { Download } from "react-feather";

// Chart configuration
const chartConfig = {
  animation: false,
  grid: { left: 20, right: 20, top: 30, bottom: 30 },
  xAxis: {
    type: "time",
    name: "Time (s)", //FIXME: Would mins be a better unit for this?
    nameTextStyle: { color: "#fff" },
    axisLabel: { color: "#fff" },
  },
  yAxis: {
    name: "Percentage (%)",
    nameTextStyle: { color: "#fff" },
    axisLabel: { color: "#fff" },
    splitLine: { show: false },
  },
  series: {
    name: "Battery Percentage",
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
        const percent = point[1].toFixed(2);

        return `${time}<br/><b>${percent}%</b>`;
      } else return ""

    },
  },
};

const BatteryChart = () => {

    const [seriesData, setSeriesData] = useState({
        time: [] as number[],
        percent: [] as number[],
    });

    //'refresh' battery state.
    const batteryStatus = useSelector(
        (state:RootState) => state.batteryStore
    );

    const addPoint = (currentData: number[], newValue: number) => {
        return [...currentData, newValue];
    };

    //Update existing data
    useEffect(() => {
        if (batteryStatus && batteryStatus.header.stamp) {
            setSeriesData((allData) => ({
                time: addPoint(allData.time, batteryStatus.header.stamp.sec * 1000 + batteryStatus.header.stamp.nanosec / 1e6),
                percent: addPoint(allData.percent, batteryStatus.percentage),
            }));
        } else return;
    }, [batteryStatus]);

    const chartRef = useRef<ReactECharts>(null);

    const chartOption = useMemo(() => {
        const data = seriesData.time.map((time, index) => [time, seriesData.percent[index]]);

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
            link.download = `battery-percentage-${Date.now()}.png`;
            link.click();
        } else return;
    }, []);

    const resetTimescale = () => {
        setSeriesData({ time: [], percent: [] });
    }

    return (
        <>
            <div className="flex items-center justify-between gap-2">
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

export default BatteryChart;
