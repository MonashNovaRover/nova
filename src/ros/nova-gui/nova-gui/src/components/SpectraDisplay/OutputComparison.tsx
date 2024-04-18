/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 * TODO:
 *  - Overhaul how graphs are overlayed to a manner that can fit single collection mode (continuous mode could just remove all overlayed graphs)
 */

import { Card, CardHeader, ScrollShadow } from "@nextui-org/react";
import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";
import DatasetChart from "./DatasetChart";
import { ChartOptions, ChartStyle } from "./ChartOptions";
import { defaultPeakFinder } from "./ChartAnalysis";

type ApexDataset = {
    name: string,
    data: number[][]
}[]

interface IOutputComparisonProps {
    outputData: ApexDataset,
    elementData: ApexDataset[],
    outputStyle: ChartStyle,
    datasetStyle: ChartStyle,
    peakFinder?: (data: number[][]) => number[][] | undefined
}

const OutputComparison: React.FC<IOutputComparisonProps> = (props: IOutputComparisonProps) => {
    const outputChartOptions: ApexOptions = ChartOptions(props.outputStyle);
    const dataChartOptions: ApexOptions = ChartOptions(props.datasetStyle);

    let peakFinder: (data: number[][]) => number[][] | undefined;
    if (props.peakFinder == undefined) {
        peakFinder = defaultPeakFinder
    } else {
        peakFinder = props.peakFinder
    }

    return (
        <Card className="w-fit p-2 m-1 w-auto">
            <CardHeader className="shrink-0 w-48 p-1">Comparison and Analysis</CardHeader>
            <div className="flex flex-row">
                <ReactApexChart className = "w-1/2 self-center" options={outputChartOptions} series={props.outputData} />
                <ScrollShadow hideScrollBar className="w-1/2 h-154">
                        {props.elementData.map( element => (<DatasetChart key={element[0].name} name={element[0].name} data={element[0].data} chartOptions={dataChartOptions} peaks={peakFinder(element[0].data)} />))}
                </ScrollShadow>
            </div>
        </Card>
    ) 
}

export default OutputComparison;