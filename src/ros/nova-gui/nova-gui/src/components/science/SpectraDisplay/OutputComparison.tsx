/**
 * This component outputs a response on a left graph, and also has graphs to compare output data against in the right half of the component.
 * Author: Connor Macdougall
 */

import { Card, CardHeader, ScrollShadow } from "@nextui-org/react";
import { ApexOptions } from "apexcharts";
import DataChart from "./DataChart.tsx";
import { ChartOptions, ChartStyle } from "./ChartOptions.ts";
import { ApexDataset } from "./DataChart.tsx";

interface IOutputComparisonProps {
    title: string,                                              // Component title
    outputData: ApexDataset,                                    // Data to be displayed in the left hand side of component ('focus'/ main screen)
    peaksOnMain?: boolean,                                      // Can omit peaks on main if it is too much of a slowdown/distraction
    elementData: ApexDataset[],                                 // Data to be displayed in the right hand side of component ('dataset'/ scrollable window for comparison)
    style: ChartStyle,                                          // Style of charts
    peakFinder?: (data: number[][]) => number[][] | undefined   // Function to return peaks found in a set of data (use getDefaultPeakFinder(resolution, peak_percentage_diff) if desired))
}

const OutputComparison: React.FC<IOutputComparisonProps> = (props: IOutputComparisonProps) => {
    const chartOptions: ApexOptions = ChartOptions(props.style);


    return (
        <Card className="w-fit p-2 m-1">
            <CardHeader className="shrink-0 w-48 p-1">{props.title}</CardHeader>
            <div className="flex flex-row">
                <div className="w-1/2 self-center">
                    <DataChart dataset={props.outputData} chartOptions={chartOptions} peaks={props.peakFinder && props.peaksOnMain ? props.peakFinder(props.outputData[0].data) : undefined } />
                </div>
                <ScrollShadow hideScrollBar className="w-1/2 h-154">
                        {props.elementData.map( element => (<div className="my-6">
                            <DataChart key={element[0].name} dataset={element} chartOptions={chartOptions} peaks={props.peakFinder ? props.peakFinder(element[0].data) : undefined } />
                        </div>))}
                </ScrollShadow>
            </div>
        </Card>
    ) 
}

export default OutputComparison;